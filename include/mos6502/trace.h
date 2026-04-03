#ifndef MOS6502_TRACE_H
#define MOS6502_TRACE_H

#include <stdio.h>

#include "mos6502/isa.h"
#include "mos6502/types.h"

#ifndef MOS6502_ENABLE_TRACE
#define MOS6502_ENABLE_TRACE 1
#endif

#if MOS6502_ENABLE_TRACE
#define TRACE_CPU_STATE(chip)                                                                 \
    do {                                                                                      \
        const uint8_t opcode = (chip)->memory[(chip)->program_counter];                       \
        printf("%04X %02X %02X %02X %02X %02X %02X\n",                                        \
               (chip)->program_counter,                                                       \
               (chip)->accumulator,                                                           \
               (chip)->index_x_register,                                                      \
               (chip)->index_y_register,                                                      \
               (chip)->stack_pointer,                                                         \
               (chip)->status_register,                                                       \
               opcode);                                                                       \
    } while (0)
#else
#define TRACE_CPU_STATE(chip)                                                                 \
    do {                                                                                      \
        (void)(chip);                                                                         \
    } while (0)
#endif

#endif //MOS6502_TRACE_H
