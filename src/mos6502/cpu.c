//
// Created by chinmay on 01/02/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mos6502/cpu.h"
#include "mos6502/interpreter_engine.h"
#include "mos6502/trace.h"


int load_program(chip_t* chip, const char* program_path) {
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

void cpu_init(chip_t *chip) {
    memset(chip, 0, sizeof(*chip));
}

void cpu_reset(chip_t* chip) {
    chip->program_counter = 0x0400;
    chip->stack_pointer = 0xFF;
    chip->status_register = 0x30;
    chip->accumulator = 0;
    chip->index_x_register = 0;
    chip->index_y_register = 0;
    memset(chip->instruction_start_map, UINT16_MAX, sizeof(chip->instruction_start_map));
    memset(chip->translation_map, UINT16_MAX, sizeof(chip->translation_map));
}

void cpu_step_interpreter(chip_t* chip) {
    TRACE_CPU_STATE(chip);

    const uint16_t pc_before = chip->program_counter;
    const uint8_t opcode = chip->memory[chip->program_counter++];

    if (opcode == 0x4C) {
        const uint8_t lo = chip->memory[chip->program_counter];
        const uint8_t hi = chip->memory[chip->program_counter + 1];

        const uint16_t target = (hi << 8) | lo;

        if (target == pc_before) {
            exit(0);
        }
    }

    execute_opcode(opcode, chip);
}

static void invalidate_translation_entry(chip_t* chip, const uint16_t spc) {
    // resetting the translation map
    if (chip->translation_map[spc] != UINT16_MAX) {
        fprintf(stderr,
            "%s (source_pc=%04X, translation_start=%04X, mapped_address=%04X)\n",
            "The translation for this SPC is being invalidated",
            spc,
            spc,
            chip->translation_map[spc]);
        chip->translation_map[spc] = UINT16_MAX;
    }
}

static void invalidate_instruction_ownership(chip_t* chip, const uint16_t spc) {
    // resetting the instruction start map
    size_t instruction_start_map_index = spc;
    if (chip->instruction_start_map[instruction_start_map_index] != UINT16_MAX) {
        while (instruction_start_map_index < CODE_CACHE_CAPACITY &&
               chip->instruction_start_map[instruction_start_map_index] == spc) {
            chip->instruction_start_map[instruction_start_map_index] = UINT16_MAX;
            instruction_start_map_index++;
               }
    }
}

static int translate_instruction(chip_t* chip, const decode_entry_t *dispatch, const uint16_t spc, const uint16_t tpc) {
    const uint8_t opcode = chip->memory[spc];

    // validity and bound checks
    if (!valid_opcode[opcode]) {
        invalidate_translation_entry(chip, spc);
        return -1;
    }

    const uint8_t length = instruction_length[opcode];
    if (spc > (uint16_t)(MEMORY_SIZE - length)) {
        invalidate_translation_entry(chip, spc);
        return -1;
    }

    // invalidating the instruction range ownership for this SPC
    invalidate_instruction_ownership(chip, spc);

    // construction of the threaded instruction
    threaded_instructions_t instruction = {
        .handler = dispatch[opcode].handler,
        .mode = dispatch[opcode].mode,
        .spc_byte_offset = length
    };
    chip->instruction_start_map[spc] = spc;
    if (length == 2) {
        instruction.operand = chip->memory[spc+1];
        chip->instruction_start_map[spc+1] = spc;
    } else if (length == 3) {
        instruction.operand = chip->memory[spc+1] | (chip->memory[spc+2] << 8);
        chip->instruction_start_map[spc+1] = spc;
        chip->instruction_start_map[spc+2] = spc;
    }
    chip->code_cache[tpc] = instruction;
    chip->translation_map[spc] = tpc;

    return length;
}

size_t cpu_translate(chip_t* chip, const decode_entry_t *dispatch) {
    /*
        this function is supposed to,
        1. read the guest instructions from the memory,
        2. construct threaded instructions based on the guest instructions,
        3. add the threaded instruction to the code cache in the chip,
        4. add the mapping from the guest program counter to the index of the corresponding threaded instruction in the code cache
    */
    uint16_t spc_translate = chip->program_counter;
    while (true) {
        const int length = translate_instruction(chip, dispatch, spc_translate, (uint16_t)chip->cache_length);
        if (length < 0) break;
        // increments
        chip->cache_length += 1;
        spc_translate += length;
        // guard against overflow
        if (chip->cache_length == CODE_CACHE_CAPACITY) break;
    }
    return chip->cache_length;
}

void cpu_run_threaded(chip_t* chip) {
    /*
        this function is supposed to,
        1. if the code cache is empty, it should call cpu_translate and fill the cache and the mapping
        2. initialize spc to 0, get the mapping from spc to tpc, get the threaded instruction for the tpc, goto the handler
        3. define labels for each operation, where,
            - the operands are used appropriately to perform the operation and update the chip state
            - the spc is incremented based on spc_byte_offset, mapped to tpc, get the threaded instruction for the tpc, goto the handler
    */
    __label__ OP_LDA, OP_LDX, OP_LDY, OP_STA, OP_STX, OP_STY, OP_ADC, OP_SBC, OP_AND, OP_ORA, OP_EOR,
              OP_BIT,
              OP_CMP, OP_CPX, OP_CPY, OP_ASL, OP_LSR, OP_ROL, OP_ROR, OP_INC, OP_DEC, OP_INX, OP_INY,
              OP_DEX, OP_DEY, OP_BCC, OP_BCS, OP_BEQ, OP_BNE, OP_BMI, OP_BPL, OP_BVC, OP_BVS, OP_JMP,
              OP_JSR, OP_RTS, OP_RTI, OP_CLC, OP_CLD, OP_CLI, OP_CLV, OP_SEC, OP_SED, OP_SEI, OP_TAX,
              OP_TAY, OP_TXA, OP_TYA, OP_TSX, OP_TXS, OP_PHA, OP_PHP, OP_PLA, OP_PLP, OP_BRK, OP_NOP;
    threaded_instructions_t instruction = {};

    // decode table to support translation
    static const decode_entry_t dispatch[256] = {
        DECODE_TABLE_CONTENT
    };

    // translation done if not done already
    if (chip->cache_length == 0) {
        cpu_translate(chip, dispatch);
    }

    // find out if the current program counter has a translation
    if (chip->translation_map[chip->program_counter] == UINT16_MAX) {
        fprintf(stderr, LOG_TRANSLATION_DOES_NOT_EXIST);
        exit(0);
    }

    // execute the threaded instruction
    instruction = chip->code_cache[chip->translation_map[chip->program_counter]];
    TRACE_CPU_STATE(chip);
    goto *instruction.handler;

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
    NEXT(chip, instruction);
}
