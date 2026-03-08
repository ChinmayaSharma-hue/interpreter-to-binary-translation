//
// Created by chinmay on 01/02/26.
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mos6502/cpu.h"

int main() {
    const char* functional_test_path = "";
    chip_t chip;
    cpu_init(&chip);

    if (load_program(&chip, functional_test_path) != 0) {
        return EXIT_FAILURE;
    }
    cpu_reset(&chip);

    char* interpreter_mode = getenv("INTERPRETER_MODE");

    if (interpreter_mode == NULL) {
        while (true) {
            cpu_step_interpreter(&chip);
        }
    }

    if (strcmp(interpreter_mode, "DIRECT_THREADED") == 0) {
        cpu_run_threaded(&chip);
    } else {
        fprintf(stderr, "ERROR: interpreter mode \"%s\" not recognized\n", interpreter_mode);
    }
}
