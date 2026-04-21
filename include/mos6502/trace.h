#ifndef MOS6502_TRACE_H
#define MOS6502_TRACE_H

#include <stdbool.h>

extern bool mos6502_trace_enabled;

#define TRACE_CPU_STATE(chip)                                                                 \
    do {                                                                                      \
        if (mos6502_trace_enabled) {                                                          \
            const uint8_t opcode = (chip)->memory[(chip)->program_counter];                   \
            printf("%04X %02X %02X %02X %02X %02X %02X\n",                                    \
                   (chip)->program_counter,                                                   \
                   (chip)->accumulator,                                                       \
                   (chip)->index_x_register,                                                  \
                   (chip)->index_y_register,                                                  \
                   (chip)->stack_pointer,                                                     \
                   (chip)->status_register,                                                   \
                   opcode);                                                                   \
        }                                                                                     \
    } while (0)

#endif //MOS6502_TRACE_H
