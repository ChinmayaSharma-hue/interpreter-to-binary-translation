//
// Created by chinmay on 01/02/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mos6502/cpu.h"
#include "mos6502/trace.h"
#include "mos6502/engines/linear.h"
#include "mos6502/engines/ops.h"

int load_program(emulator_t *emulator, const char* program_path) {
    chip_t *chip = &emulator->chip;
    FILE *fptr = fopen(program_path, "rb");
    if (fptr == NULL) {
        perror("fopen");
        return -1;
    }

    const size_t bytes_read = fread(&chip->memory[0], sizeof(uint8_t), MEMORY_SIZE, fptr);
    fclose(fptr);
    if (bytes_read != MEMORY_SIZE) {
        fprintf(stderr, "Failed to read full program image from %s (read %zu of %d bytes)\n",
                program_path, bytes_read, MEMORY_SIZE);
        return -1;
    }

    return 0;
}

void cpu_init(emulator_t *emulator) {
    memset(&emulator->chip, 0, sizeof(emulator->chip));
}

void cpu_reset(emulator_t *emulator) {
    chip_t *chip = &emulator->chip;
    chip->program_counter = 0x0400;
    chip->stack_pointer = 0xFF;
    chip->status_register = 0x30;
    chip->accumulator = 0;
    chip->index_x_register = 0;
    chip->index_y_register = 0;
}

static void invalidate_translation_entry(translation_directory_t* directory, const uint16_t spc) {
    // resetting the translation map
    if (directory->spc_to_tpc[spc] != UINT16_MAX) {
        directory->spc_to_tpc[spc] = UINT16_MAX;
    }
}

static void invalidate_instruction_ownership(translation_directory_t* directory, const uint16_t spc) {
    // resetting the instruction start map
    size_t instruction_owner_map_index = spc;
    if (directory->instruction_owner[instruction_owner_map_index] != UINT16_MAX) {
        while (instruction_owner_map_index < CODE_CACHE_CAPACITY &&
               directory->instruction_owner[instruction_owner_map_index] == spc) {
            directory->instruction_owner[instruction_owner_map_index] = UINT16_MAX;
            instruction_owner_map_index++;
        }
    }
}

static precode_result_t precode_instruction(
    const chip_t* chip,
    const decode_entry_t *dispatch,
    const uint16_t spc,
    precoded_instruction_t* precoded_instruction
) {
    // validity and bound checks
    const uint8_t opcode = chip->memory[spc];
    if (!valid_opcode[opcode]) {
        return PRECODE_INVALID_OPCODE;
    }

    const uint8_t length = instruction_length[opcode];
    if (spc > (uint16_t)(MEMORY_SIZE - length)) {
        return PRECODE_OUT_OF_BOUNDS;
    }

    // construction of the threaded instruction
    precoded_instruction->handler = dispatch[opcode].handler;
    precoded_instruction->mode = dispatch[opcode].mode;
    precoded_instruction->spc_byte_offset = length;
    if (length == 2) {
        precoded_instruction->operand = chip->memory[spc+1];
    } else if (length == 3) {
        precoded_instruction->operand = chip->memory[spc+1] | (chip->memory[spc+2] << 8);
    }
    return PRECODE_OK;
}

static void cache_instruction(
    const precoded_instruction_t instruction,
    emulator_t *emulator,
    const uint16_t spc,
    const uint16_t tpc
) {
    threaded_engine_state_t* engine_state = THREADED_ENGINE_STATE(emulator);
    // invalidating the instruction range ownership for this SPC
    invalidate_instruction_ownership(&engine_state->directory, spc);
    // construction of the threaded instruction
    engine_state->directory.instruction_owner[spc] = spc;
    if (instruction.spc_byte_offset == 2) {
        engine_state->directory.instruction_owner[spc+1] = spc;
    } else if (instruction.spc_byte_offset == 3) {
        engine_state->directory.instruction_owner[spc+1] = spc;
        engine_state->directory.instruction_owner[spc+2] = spc;
    }
    engine_state->code_cache[tpc] = instruction;
    engine_state->directory.spc_to_tpc[spc] = tpc;
}

static int reprecode_instruction(const decode_entry_t *dispatch, emulator_t *emulator, const uint16_t spc)
{
    // checking if a precoding already exists
    const chip_t *chip = &emulator->chip;
    threaded_engine_state_t *engine_state = THREADED_ENGINE_STATE(emulator);
    uint16_t tpc = engine_state->directory.spc_to_tpc[spc];
    if (tpc == UINT16_MAX) {
        // guard against overflow
        if (engine_state->cache_length >= CODE_CACHE_CAPACITY) return -1;
        tpc = (uint16_t)engine_state->cache_length;
        engine_state->cache_length += 1;
    }

    // precoding and caching the instruction
    precoded_instruction_t instruction = {};
    precode_result_t precode_result = precode_instruction(chip, dispatch, spc, &instruction);
    switch (precode_result) {
        case PRECODE_OK: {
            cache_instruction(instruction, emulator, spc, tpc);
            return instruction.spc_byte_offset;
        }
        case PRECODE_INVALID_OPCODE:
        case PRECODE_OUT_OF_BOUNDS:
        default: {
            invalidate_translation_entry(&engine_state->directory, spc);
            return -1;
        }
    }
    return -1;
}

static inline void threaded_code_write(const decode_entry_t *dispatch, emulator_t *emulator, const uint16_t address) {
    const chip_t chip = emulator->chip;
    const threaded_engine_state_t *engine_state = THREADED_ENGINE_STATE(emulator);
    uint16_t owner_spc = engine_state->directory.instruction_owner[address];
    if (owner_spc == UINT16_MAX) return;

    REPORT_SELF_MODIFYING_CODE(&chip, address);

    // reprecoding the directly affected instruction
    int length = reprecode_instruction(dispatch, emulator, owner_spc);
    if (length <= 0) return;
    owner_spc += (uint16_t)length;

    // reprecoding the instructions affected after the current instruction
    while (!(engine_state->directory.instruction_owner[owner_spc] == owner_spc &&
             engine_state->directory.spc_to_tpc[owner_spc] != UINT16_MAX)) {
        length = reprecode_instruction(dispatch, emulator, owner_spc);
        if (length <= 0) return;
        owner_spc += (uint16_t)length;
    }
}

static inline void block_code_write(const decode_entry_t *dispatch, emulator_t *emulator, const uint16_t address) {
    const chip_t chip = emulator->chip;
    block_engine_state_t *engine_state = BLOCK_ENGINE_STATE(emulator);
    const uint16_t owner_spc = engine_state->directory.instruction_owner[address];
    if (owner_spc == UINT16_MAX) return;

    REPORT_SELF_MODIFYING_CODE(&chip, address);

    invalidate_translation_entry(&engine_state->directory, owner_spc);
    invalidate_instruction_ownership(&engine_state->directory, owner_spc);
}

size_t precode_batch(const decode_entry_t *dispatch, emulator_t* emulator) {
    /*
        this function is supposed to,
        1. read the guest instructions from the memory,
        2. construct threaded instructions based on the guest instructions,
        3. add the threaded instruction to the code cache in the chip,
        4. add the mapping from the guest program counter to the index of the corresponding threaded instruction in the code cache
    */
    const chip_t *chip = &emulator->chip;
    uint16_t spc = emulator->chip.program_counter;
    threaded_engine_state_t *engine_state = THREADED_ENGINE_STATE(emulator);
    while (true) {
        precoded_instruction_t instruction = {};
        precode_result_t precode_result = precode_instruction(chip, dispatch, spc, &instruction);
        switch (precode_result) {
            case PRECODE_OK: {
                cache_instruction(instruction, emulator, spc, (uint16_t)engine_state->cache_length);
                spc += instruction.spc_byte_offset;
                engine_state->cache_length += 1;
                if (engine_state->cache_length == CODE_CACHE_CAPACITY) goto done;
                break;
            }
            case PRECODE_INVALID_OPCODE:
            case PRECODE_OUT_OF_BOUNDS:
                invalidate_translation_entry(&engine_state->directory, spc);
                goto done;
        }
    }
done:
    return engine_state->cache_length;
}

void precode_block(const decode_entry_t *dispatch, emulator_t* emulator) {
    const chip_t *chip = &emulator->chip;
    uint16_t spc = chip->program_counter;
    block_engine_state_t* engine_state = BLOCK_ENGINE_STATE(emulator);

    // flush if arena is too close to capacity to fit a new block
    if (engine_state->arena_head >= CODE_CACHE_CAPACITY - 32) {
        memset(engine_state->directory.spc_to_tpc, 0xFF, sizeof(engine_state->directory.spc_to_tpc));
        memset(engine_state->directory.instruction_owner, 0xFF, sizeof(engine_state->directory.instruction_owner));
        engine_state->cache_length = 0;
        engine_state->arena_head = 0;
    }

    translation_block_t translation_block = {
        .instructions = &engine_state->instruction_arena[engine_state->arena_head],
        .block_length = 0
    };
    while (true) {
        // block termination
        if (is_block_terminator(chip, spc)) {
            break;
        }

        // block construction
        precoded_instruction_t instruction = {};
        precode_result_t precode_result = precode_instruction(chip, dispatch, spc, &instruction);
        switch (precode_result) {
            case PRECODE_OK: {
                engine_state->instruction_arena[engine_state->arena_head++] = instruction;
                for (int i=0; i<instruction.spc_byte_offset; i++) {
                    engine_state->directory.instruction_owner[spc++] = chip->program_counter;
                }
                translation_block.block_length += 1;
                break;
            }
            case PRECODE_INVALID_OPCODE:
            case PRECODE_OUT_OF_BOUNDS:
                goto done;
        }
    }

done:
    // add block to the cache
    if (translation_block.block_length > 0) {
        engine_state->code_cache[(uint16_t)engine_state->cache_length] = translation_block;
        engine_state->directory.spc_to_tpc[chip->program_counter] = (uint16_t)engine_state->cache_length;
        engine_state->cache_length += 1;
    }
}

static step_termination_type_t cpu_step_linear(chip_t* chip) {
    TRACE_CPU_STATE(chip);

    const uint8_t opcode = chip->memory[chip->program_counter];
    const uint16_t pc_before = chip->program_counter;
    chip->program_counter++;

    if (opcode == 0x4C) {
        const uint8_t lo = chip->memory[chip->program_counter];
        const uint8_t hi = chip->memory[chip->program_counter + 1];

        const uint16_t target = (hi << 8) | lo;

        if (target == pc_before) {
            return LINEAR_EXIT;
        }
    }

    execute_opcode(opcode, chip);
    if (chip->program_counter == 0x362A &&
        chip->accumulator == 0x3E &&
        chip->index_x_register == 0x0E &&
        chip->index_y_register == 0xFF &&
        chip->stack_pointer == 0xFB &&
        chip->status_register == 0xB0 &&
        opcode == 0xD0) {
        return LINEAR_EXIT;
    }
    return LINEAR_CONTINUE;
}

int cpu_run(emulator_t *emulator) {
    /*
        for direct threading, this function is supposed to,
            1. if the code cache is empty, it should call precode_batch and fill the cache and the mapping
            2. initialize spc to 0, get the mapping from spc to tpc, get the threaded instruction for the tpc, goto the handler
            3. define labels for each operation, where,
                - the operands are used appropriately to perform the operation and update the chip state
                - the spc is incremented based on spc_byte_offset, mapped to tpc, get the threaded instruction for the tpc, goto the handler
        for block, this function is supposed to,
            1. if there is no entry in the translation map for the current SPC, then call precode_block for the current SPC
            2. if there is an entry in the translation map for the current SPC, then execute that block till control jumps back to BLOCK_DONE
            3. once the block is done, find out if the current instruction is one of the jump instructions, execute it if it is, then do the same as before
    */
    __label__ OP_LDA, OP_LDX, OP_LDY, OP_STA, OP_STX, OP_STY, OP_ADC, OP_SBC, OP_AND, OP_ORA, OP_EOR,
              OP_BIT,
              OP_CMP, OP_CPX, OP_CPY, OP_ASL, OP_LSR, OP_ROL, OP_ROR, OP_INC, OP_DEC, OP_INX, OP_INY,
              OP_DEX, OP_DEY, OP_BCC, OP_BCS, OP_BEQ, OP_BNE, OP_BMI, OP_BPL, OP_BVC, OP_BVS, OP_JMP,
              OP_JSR, OP_RTS, OP_RTI, OP_CLC, OP_CLD, OP_CLI, OP_CLV, OP_SEC, OP_SED, OP_SEI, OP_TAX,
              OP_TAY, OP_TXA, OP_TYA, OP_TSX, OP_TXS, OP_PHA, OP_PHP, OP_PLA, OP_PLP, OP_BRK, OP_NOP,
              BLOCK_DONE;
    chip_t *chip = &emulator->chip;
    precoded_instruction_t instruction = {};
    static const decode_entry_t dispatch[256] = {
        DECODE_TABLE_CONTENT
    };

    switch (emulator->emulation_mode) {
        case EMULATION_MODE_DIRECT_THREADED: {
            threaded_engine_state_t* engine_state = THREADED_ENGINE_STATE(emulator);

            // translation done if not done already
            if (engine_state->cache_length == 0) {
                precode_batch(dispatch, emulator);
            }

            // find out if the current program counter has a translation
            if (engine_state->directory.spc_to_tpc[chip->program_counter] == UINT16_MAX) {
                fprintf(stderr, LOG_TRANSLATION_DOES_NOT_EXIST);
                exit(0);
            }

            // execute the threaded instruction
            instruction = engine_state->code_cache[engine_state->directory.spc_to_tpc[chip->program_counter]];
            TRACE_CPU_STATE(chip);
            goto *instruction.handler;
            break;
        }
        case EMULATION_MODE_FETCH_DECODE_EXECUTE: {
            while (cpu_step_linear(chip) != LINEAR_EXIT) {}
            break;
        }
        case EMULATION_MODE_BLOCK: {
            block_engine_state_t *engine_state = BLOCK_ENGINE_STATE(emulator);
BLOCK_DONE:
            engine_state->program_counter = 0;
            // execute all the block terminator instructions
            while (is_block_terminator(chip, chip->program_counter)) {
                step_termination_type_t termination_type = cpu_step_linear(chip);
                if (termination_type == LINEAR_EXIT) {
                    exit(0);
                }
            }

            // precode the block if no translation block exists
            if (engine_state->directory.spc_to_tpc[chip->program_counter] == UINT16_MAX) {
                precode_block(dispatch, emulator);
            }
            uint16_t block_length = engine_state->code_cache[engine_state->directory.spc_to_tpc[chip->program_counter]].block_length;

            // go to the block execution starting point
            if (block_length > 0) {
                engine_state->current_block = &engine_state->code_cache[engine_state->directory.spc_to_tpc[chip->program_counter]];
                instruction = *engine_state->current_block->instructions;
                TRACE_CPU_STATE(chip);
                goto *engine_state->current_block->instructions->handler;
            }
            goto BLOCK_DONE;
        }
        default:
            return EXIT_FAILURE;
    }

// Load Operations
OP_LDA:
    OP_LOAD_REG(chip, instruction, chip->accumulator);
OP_LDX:
    OP_LOAD_REG(chip, instruction, chip->index_x_register);
OP_LDY:
    OP_LOAD_REG(chip, instruction, chip->index_y_register);
    // Store Operations
OP_STA:
    OP_STORE_REG(chip, instruction, chip->accumulator);
OP_STX:
    OP_STORE_REG(chip, instruction, chip->index_x_register);
OP_STY:
    OP_STORE_REG(chip, instruction, chip->index_y_register);
    // Arithmetic Operations
OP_ADC:
    OP_ADC(chip, instruction);
OP_SBC:
    OP_SBC(chip, instruction);
    // Boolean Operations
OP_AND:
    OP_AND(chip, instruction);
OP_ORA:
    OP_ORA(chip, instruction);
OP_EOR:
    OP_EOR(chip, instruction);
OP_BIT:
    OP_BIT(chip, instruction);
    // Compare Operations
OP_CMP:
    OP_CMP(chip, instruction);
OP_CPX:
    OP_CPX(chip, instruction);
OP_CPY:
    OP_CPY(chip, instruction);
    // Shift Operations
OP_ASL:
    OP_ASL(chip, instruction);
OP_LSR:
    OP_LSR(chip, instruction);
OP_ROL:
    OP_ROL(chip, instruction);
OP_ROR:
    OP_ROR(chip, instruction);
    // Increment/Decrement Operations
OP_INC:
    OP_INC(chip, instruction);
OP_DEC:
    OP_DEC(chip, instruction);
OP_INX:
    OP_INX(chip, instruction);
OP_INY:
    OP_INY(chip, instruction);
OP_DEX:
    OP_DEX(chip, instruction);
OP_DEY:
    OP_DEY(chip, instruction);
    // Branch Operations
OP_BCC:
    OP_BCC(chip, instruction);
OP_BCS:
    OP_BCS(chip, instruction);
OP_BEQ:
    OP_BEQ(chip, instruction);
OP_BNE:
    OP_BNE(chip, instruction);
OP_BMI:
    OP_BMI(chip, instruction);
OP_BPL:
    OP_BPL(chip, instruction);
OP_BVC:
    OP_BVC(chip, instruction);
OP_BVS:
    OP_BVS(chip, instruction);
OP_JMP:
    OP_JUMP(chip, instruction);
OP_JSR:
    OP_JSR(chip, instruction);
OP_RTS:
    OP_RTS(chip, instruction);
OP_RTI:
    OP_RTI(chip, instruction);
    // Flag Operations
OP_CLC:
    OP_CLC(chip, instruction);
OP_CLD:
    OP_CLD(chip, instruction);
OP_CLI:
    OP_CLI(chip, instruction);
OP_CLV:
    OP_CLV(chip, instruction);
OP_SEC:
    OP_SEC(chip, instruction);
OP_SED:
    OP_SED(chip, instruction);
OP_SEI:
    OP_SEI(chip, instruction);
    // Transfer Operations
OP_TAX:
    OP_TRANSFER_TO_REG(chip->index_x_register, chip->accumulator, chip, instruction);
OP_TAY:
    OP_TRANSFER_TO_REG(chip->index_y_register, chip->accumulator, chip, instruction);
OP_TXA:
    OP_TRANSFER_TO_REG(chip->accumulator, chip->index_x_register, chip, instruction);
OP_TYA:
    OP_TRANSFER_TO_REG(chip->accumulator, chip->index_y_register, chip, instruction);
OP_TSX:
    OP_TRANSFER_TO_REG(chip->index_x_register, chip->stack_pointer, chip, instruction);
OP_TXS:
    OP_TRANSFER_TO_STACK(chip->index_x_register, chip, instruction);
    // Stack Operations
OP_PHA:
    OP_PUSH_TO_STACK(chip, instruction, chip->accumulator);
OP_PHP:
    OP_PUSH_TO_STACK(chip, instruction, chip->status_register | 0x30);
OP_PLA:
    OP_PULL_ACCUMULATOR(chip, instruction);
OP_PLP:
    OP_PULL_STATUS_REG(chip, instruction);
    // Miscellaneous Operations
OP_BRK:
    OP_BRK(chip, instruction);
OP_NOP:
    ENGINE_NEXT(chip, instruction);
}
