#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/emulator.h"
#include "mos6502/cpu.h"

typedef enum {
    EMULATION_MODE_FETCH_DECODE_EXECUTE = 0,
    EMULATION_MODE_DIRECT_THREADED
} emulation_mode_t;

typedef struct {
    const char *program_path;
    const char *mode_from_cli;
} app_config_t;

static const char *const DEFAULT_PROGRAM_PATH = "tests/functional/6502_functional_test.bin";

static void print_usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s [--program <path>] [--mode <FETCH_DECODE_EXECUTE|DIRECT_THREADED>]\n"
            "\n"
            "Options:\n"
            "  --program <path>  Path to 6502 program binary.\n"
            "  --mode <value>    Emulation mode. Defaults to EMULATION_MODE env var or FETCH_DECODE_EXECUTE.\n"
            "  -h, --help        Show this help text.\n",
            argv0);
}

static bool parse_emulation_mode(const char *value, emulation_mode_t *mode, bool allow_empty_default) {
    if ((value == NULL || value[0] == '\0') && allow_empty_default) {
        *mode = EMULATION_MODE_FETCH_DECODE_EXECUTE;
        return true;
    }

    if (strcmp(value, "FETCH_DECODE_EXECUTE") == 0) {
        *mode = EMULATION_MODE_FETCH_DECODE_EXECUTE;
        return true;
    }

    if (strcmp(value, "DIRECT_THREADED") == 0) {
        *mode = EMULATION_MODE_DIRECT_THREADED;
        return true;
    }

    return false;
}

static void init_default_config(app_config_t *config) {
    config->program_path = DEFAULT_PROGRAM_PATH;
    config->mode_from_cli = NULL;
}

static int parse_cli_args(int argc, char **argv, app_config_t *config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        if (strcmp(argv[i], "--program") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "ERROR: --program requires a value.\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            config->program_path = argv[++i];
            continue;
        }

        if (strcmp(argv[i], "--mode") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "ERROR: --mode requires a value.\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            config->mode_from_cli = argv[++i];
            continue;
        }

        fprintf(stderr, "ERROR: Unknown argument: %s\n", argv[i]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return -1;
}

static int resolve_emulation_mode(const app_config_t *config, emulation_mode_t *mode) {
    if (config->mode_from_cli != NULL) {
        if (!parse_emulation_mode(config->mode_from_cli, mode, false)) {
            fprintf(stderr,
                    "ERROR: --mode=\"%s\" is invalid. Allowed values: FETCH_DECODE_EXECUTE, DIRECT_THREADED\n",
                    config->mode_from_cli);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    const char *emulation_mode_env = getenv("EMULATION_MODE");
    if (!parse_emulation_mode(emulation_mode_env, mode, true)) {
        fprintf(stderr,
                "ERROR: EMULATION_MODE=\"%s\" is invalid. Allowed values: FETCH_DECODE_EXECUTE, DIRECT_THREADED\n",
                emulation_mode_env);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int initialize_cpu(chip_t *chip, const char *program_path) {
    cpu_init(chip);

    if (load_program(chip, program_path) != 0) {
        fprintf(stderr, "ERROR: Failed to load program: %s\n", program_path);
        return EXIT_FAILURE;
    }

    cpu_reset(chip);
    return EXIT_SUCCESS;
}

static int run_emulation(chip_t *chip, emulation_mode_t mode) {
    if (mode == EMULATION_MODE_FETCH_DECODE_EXECUTE) {
        while (true) {
            cpu_step_interpreter(chip);
        }
    }

    if (mode == EMULATION_MODE_DIRECT_THREADED) {
        cpu_run_threaded(chip);
        return EXIT_SUCCESS;
    }

    fprintf(stderr,
            "ERROR: Unsupported mode selected (%d). Allowed values: FETCH_DECODE_EXECUTE, DIRECT_THREADED\n",
            mode);
    return EXIT_FAILURE;
}

int app_run(int argc, char **argv) {
    app_config_t config;
    chip_t chip;
    emulation_mode_t mode;

    init_default_config(&config);

    const int parse_result = parse_cli_args(argc, argv, &config);
    if (parse_result != -1) {
        return parse_result;
    }

    if (initialize_cpu(&chip, config.program_path) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (resolve_emulation_mode(&config, &mode) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return run_emulation(&chip, mode);
}
