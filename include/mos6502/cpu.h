//
// Created by chinmay on 01/02/26.
//

#ifndef MOS6502_CPU_H
#define MOS6502_CPU_H

#include "emulator.h"
#include "mos6502/types.h"

#define MEMORY_SIZE 65536
#define ENTRY_POINT 0x0000

typedef struct emulator_t emulator_t;

typedef enum {
    PRECODE_OK,
    PRECODE_INVALID_OPCODE,
    PRECODE_OUT_OF_BOUNDS
} precode_result_t;

void cpu_init(emulator_t *emulator);
void cpu_reset(emulator_t *emulator);
size_t precode_batch(
    const decode_entry_t *dispatch,
    emulator_t *emulator
);
int cpu_run(emulator_t *emulator);
int load_program(emulator_t *emulator, const char* program_path);

#endif //MOS6502_CPU_H
