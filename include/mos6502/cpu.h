//
// Created by chinmay on 01/02/26.
//

#ifndef MOS6502_CPU_H
#define MOS6502_CPU_H
#include "mos6502/types.h"
#include "mos6502/exec.h"

#define CODE_CACHE_CAPACITY 65535
#define MEMORY_SIZE 65536
#define ENTRY_POINT 0x0000

void cpu_init(chip_t *chip);
void cpu_reset(chip_t* chip);
void cpu_step_interpreter(chip_t* chip);
void cpu_translate(chip_t* chip, const decode_entry_t *dispatch);
void cpu_run_threaded(chip_t *chip);
int load_program(chip_t* chip, const char* program_path);


#endif //MOS6502_CPU_H
