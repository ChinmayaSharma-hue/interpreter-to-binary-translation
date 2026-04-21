#ifndef MOS6502_BLOCK_ENGINE_H
#define MOS6502_BLOCK_ENGINE_H

#include <stdbool.h>
#include <stdlib.h>

#include "mos6502/cpu.h"
#include "mos6502/isa.h"
#include "mos6502/types.h"

typedef struct emulator_t emulator_t;

#define BLOCK_ENGINE_STATE(emulator) (&((emulator)->engine.block))

#define BLOCK_REPORT_INSTRUCTION_DOES_NOT_EXIST(chip)                                       \
    do {                                                                                    \
        uint16_t source_program_counter = (chip)->program_counter;                          \
        uint16_t mapped_address = BLOCK_ENGINE_STATE(emulator)->directory.spc_to_tpc[source_program_counter]; \
        fprintf(stderr,                                                                     \
                "%s (source_pc=%04X, mapped_address=%04X)\n",                              \
                LOG_INSTRUCTION_DOES_NOT_EXIST,                                             \
                source_program_counter,                                                     \
                mapped_address);                                                            \
    } while (0)

#define BLOCK_JUMP(chip, instruction, address) goto DISPATCHER;

#define BLOCK_CODE_WRITE(chip, address)                                                     \
    block_code_write(dispatch, emulator, (address))


#define MAX_BLOCKS_PER_ADDR 8

typedef struct translation_block_t {
    precoded_instruction_t *instructions;
    size_t block_length;
    bool valid;
} translation_block_t;

typedef struct {
    translation_directory_t directory;
    translation_block_t code_cache[CODE_CACHE_CAPACITY];
    size_t cache_length;
    precoded_instruction_t instruction_arena[CODE_CACHE_CAPACITY];
    uint16_t arena_head;
} block_engine_state_t;

#define BLOCK_NEXT(chip, instruction)                                                                \
    do {                                                                                             \
        (chip)->program_counter += (instruction)->spc_byte_offset;                                   \
        (instruction)++;                                                                             \
        if ((instruction)->spc_byte_offset != 0) {                                                   \
            TRACE_CPU_STATE(chip);                                                                   \
        }                                                                                            \
        goto *(instruction)->handler;                                                                \
    } while (0)

static bool inline is_block_terminator(const chip_t* chip, const uint16_t program_counter) {
    const instruction_type_t type = instruction_type_by_opcode[chip->memory[program_counter]];

    return (
      type == INSTRUCTION_BRANCH_IF_CARRY_CLEAR ||
      type == INSTRUCTION_BRANCH_IF_CARRY_SET ||
      type == INSTRUCTION_BRANCH_IF_EQUAL ||
      type == INSTRUCTION_BRANCH_IF_MINUS ||
      type == INSTRUCTION_BRANCH_IF_NOT_EQUAL ||
      type == INSTRUCTION_BRANCH_IF_OVERFLOW_CLEAR ||
      type == INSTRUCTION_BRANCH_IF_OVERFLOW_SET ||
      type == INSTRUCTION_BRANCH_IF_POSITIVE ||
      type == INSTRUCTION_JUMP ||
      type == INSTRUCTION_JUMP_TO_SUBROUTINE ||
      type == INSTRUCTION_RETURN_FROM_INTERRUPT ||
      type == INSTRUCTION_RETURN_FROM_SUBROUTINE ||
      type == INSTRUCTION_FORCE_BREAK
    );
};

#endif
