#ifndef MOS6502_THREADED_ENGINE_H
#define MOS6502_THREADED_ENGINE_H

#include "mos6502/trace.h"
#include "mos6502/engines/ops.h"

#include <stdio.h>

typedef struct emulator_t emulator_t;

#define THREADED_ENGINE_STATE(emulator) (&((emulator)->engine.threaded))

#define THREADED_REPORT_INSTRUCTION_DOES_NOT_EXIST(chip)                                    \
    do {                                                                                    \
        uint16_t source_program_counter = (chip)->program_counter;                          \
        uint16_t mapped_address = THREADED_ENGINE_STATE(emulator)->directory.spc_to_tpc[source_program_counter]; \
        fprintf(stderr,                                                                     \
                "%s (source_pc=%04X, mapped_address=%04X)\n",                              \
                LOG_INSTRUCTION_DOES_NOT_EXIST,                                             \
                source_program_counter,                                                     \
                mapped_address);                                                            \
    } while (0)

#define THREADED_NEXT(chip, instruction)                                                     \
    do {                                                                                     \
        (chip)->program_counter += (instruction).spc_byte_offset;                            \
        uint16_t mapped_address = THREADED_ENGINE_STATE(emulator)->directory.spc_to_tpc[(chip)->program_counter]; \
        if (mapped_address == UINT16_MAX) {                                                  \
            size_t previous_cache_length = THREADED_ENGINE_STATE(emulator)->cache_length;    \
            if (previous_cache_length == precode_batch(dispatch, emulator)) {          \
                REPORT_TRANSLATION_DOES_NOT_EXIST(chip, mapped_address);                     \
                exit(0);                                                                     \
            }                                                                                \
        }                                                                                    \
        mapped_address = THREADED_ENGINE_STATE(emulator)->directory.spc_to_tpc[(chip)->program_counter]; \
        (instruction) = THREADED_ENGINE_STATE(emulator)->code_cache[mapped_address];         \
        TRACE_CPU_STATE(chip);                                                               \
        goto *(instruction).handler;                                                         \
    } while (0)

#define THREADED_JUMP(chip, instruction, address)                                            \
    do {                                                                                     \
        (chip)->program_counter = (address);                                                 \
        uint16_t mapped_address = THREADED_ENGINE_STATE(emulator)->directory.spc_to_tpc[(chip)->program_counter]; \
        if (mapped_address == UINT16_MAX) {                                                  \
            size_t previous_cache_length = THREADED_ENGINE_STATE(emulator)->cache_length;    \
            if (previous_cache_length == precode_batch(dispatch, emulator)) {          \
                REPORT_TRANSLATION_DOES_NOT_EXIST(chip, mapped_address);                     \
                exit(0);                                                                     \
            }                                                                                \
        }                                                                                    \
        mapped_address = THREADED_ENGINE_STATE(emulator)->directory.spc_to_tpc[(chip)->program_counter]; \
        (instruction) = THREADED_ENGINE_STATE(emulator)->code_cache[mapped_address];         \
        TRACE_CPU_STATE(chip);                                                               \
        goto *(instruction).handler;                                                         \
    } while (0)

#define THREADED_CODE_WRITE(chip, address) \
    threaded_code_write(dispatch, emulator, (address))

typedef struct {
    translation_directory_t directory;
    precoded_instruction_t code_cache[CODE_CACHE_CAPACITY];
    size_t cache_length;
} threaded_engine_state_t;

typedef int (*translate_instruction_fn_t)(
    chip_t *chip,
    const decode_entry_t *dispatch,
    emulator_t *emulator,
    uint16_t spc,
    uint16_t tpc);
#endif
