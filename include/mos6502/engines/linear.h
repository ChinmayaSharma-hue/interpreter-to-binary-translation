//
// Created by chinmay on 01/02/26.
//

#ifndef MOS6502_INTERPRETER_ENGINE_H
#define MOS6502_INTERPRETER_ENGINE_H

#include "mos6502/isa.h"
#include "mos6502/types.h"

typedef void (*opcode_func)(chip_t *);

typedef enum {
    LINEAR_EXIT,
    LINEAR_CONTINUE
} step_termination_type_t;

static step_termination_type_t cpu_step_linear(chip_t* chip);
void execute_opcode(uint8_t opcode, chip_t *chip);

#endif //MOS6502_INTERPRETER_ENGINE_H
