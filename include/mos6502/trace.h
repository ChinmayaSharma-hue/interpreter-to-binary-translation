#ifndef MOS6502_TRACE_H
#define MOS6502_TRACE_H

#include <stdio.h>

#include "mos6502/types.h"

#define TRACE_CPU_STATE(chip)                                                                 \
    do {                                                                                      \
        printf("%04X %02X %02X %02X %02X %02X %02X\n",                                       \
               (chip)->program_counter,                                                       \
               (chip)->accumulator,                                                           \
               (chip)->index_x_register,                                                      \
               (chip)->index_y_register,                                                      \
               (chip)->stack_pointer,                                                         \
               (chip)->status_register,                                                       \
               (chip)->memory[(chip)->program_counter]);                                      \
    } while (0)

#endif //MOS6502_TRACE_H
