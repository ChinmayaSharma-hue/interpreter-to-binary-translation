#ifndef APP_EMULATOR_APP_H
#define APP_EMULATOR_APP_H

#include "mos6502/engines/block.h"
#include "mos6502/engines/threaded.h"

typedef enum {
    EMULATION_MODE_FETCH_DECODE_EXECUTE = 0,
    EMULATION_MODE_DIRECT_THREADED = 1,
    EMULATION_MODE_BLOCK = 2
} emulation_mode_t;

typedef struct emulator_t {
    chip_t chip;
    emulation_mode_t emulation_mode;
    union {
        threaded_engine_state_t threaded;
        block_engine_state_t block;
    } engine;
} emulator_t;

int run(int argc, char **argv);

#endif
