#ifndef MOS6502_TYPES_H
#define MOS6502_TYPES_H
#include <stddef.h>
#include <stdint.h>

#define CODE_CACHE_CAPACITY 65536

typedef enum {
    AM_IMP,
    AM_ACC,
    AM_IMM,
    AM_ZP,
    AM_ZP_X,
    AM_ZP_Y,
    AM_ABS,
    AM_ABS_X,
    AM_ABS_Y,
    AM_IND,
    AM_IND_X,
    AM_IND_Y,
    AM_REL
} addressing_mode_t;

typedef struct {
    void* handler;
    addressing_mode_t mode;
} decode_entry_t;

typedef struct {
    void *handler;
    addressing_mode_t mode;
    uint16_t operand;
    uint8_t spc_byte_offset;
} threaded_instructions_t;

typedef struct {
    uint8_t accumulator;
    uint8_t index_x_register;
    uint8_t index_y_register;
    uint8_t status_register;
    uint8_t stack_pointer;
    uint16_t program_counter;
    uint8_t memory[65536];
    size_t cache_length;
    threaded_instructions_t code_cache[CODE_CACHE_CAPACITY];
    uint16_t translation_map[CODE_CACHE_CAPACITY];
    uint16_t instruction_start_map[CODE_CACHE_CAPACITY];
} chip_t;
#endif
