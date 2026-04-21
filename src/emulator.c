#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emulator.h"
#include "mos6502/cpu.h"
#include "mos6502/trace.h"

typedef struct {
    const char *program_path;
    const char *mode_from_cli;
} config_t;

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

    if (strcmp(value, "BLOCK") == 0) {
        *mode = EMULATION_MODE_BLOCK;
        return true;
    }

    return false;
}

static void init_default_config(config_t *config) {
    config->program_path = DEFAULT_PROGRAM_PATH;
    config->mode_from_cli = NULL;
}

static int parse_cli_args(int argc, char **argv, config_t *config) {
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

static int resolve_emulation_mode(const config_t *config, emulation_mode_t *mode) {
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

static int initialize_cpu(emulator_t *emulator, const char *program_path) {
    cpu_init(emulator);

    if (load_program(emulator, program_path) != 0) {
        fprintf(stderr, "ERROR: Failed to load program: %s\n", program_path);
        return EXIT_FAILURE;
    }

    cpu_reset(emulator);
    return EXIT_SUCCESS;
}

static int initialize_emulator(emulator_t *emulator, const emulation_mode_t mode) {
    memset(emulator, 0, sizeof(*emulator));

    switch (mode) {
        case EMULATION_MODE_FETCH_DECODE_EXECUTE:
            emulator->emulation_mode = EMULATION_MODE_FETCH_DECODE_EXECUTE;
            return EXIT_SUCCESS;

        case EMULATION_MODE_DIRECT_THREADED: {
            emulator->emulation_mode = EMULATION_MODE_DIRECT_THREADED;
            emulator->engine.threaded.cache_length = 0;
            memset(emulator->engine.threaded.directory.instruction_owner, UINT16_MAX, sizeof(emulator->engine.threaded.directory.instruction_owner));
            memset(emulator->engine.threaded.directory.spc_to_tpc, UINT16_MAX, sizeof(emulator->engine.threaded.directory.spc_to_tpc));
            return EXIT_SUCCESS;
        }
        case EMULATION_MODE_BLOCK: {
            emulator->emulation_mode = EMULATION_MODE_BLOCK;
            emulator->engine.block.arena_head = 0;
            emulator->engine.block.cache_length = 0;
            memset(emulator->engine.block.directory.instruction_owner, UINT16_MAX, sizeof(emulator->engine.block.directory.instruction_owner));
            memset(emulator->engine.block.directory.spc_to_tpc, UINT16_MAX, sizeof(emulator->engine.block.directory.spc_to_tpc));
            return EXIT_SUCCESS;
        }

        default:
            fprintf(stderr,
                    "ERROR: Unsupported mode selected (%d). Allowed values: FETCH_DECODE_EXECUTE, DIRECT_THREADED\n",
                    mode);
            return EXIT_FAILURE;
    }
}

static int run_emulation(emulator_t *emulator) {
    if (cpu_run(emulator) != EXIT_SUCCESS) {
        fprintf(stderr,
            "ERROR: Unsupported mode selected (%d). Allowed values: FETCH_DECODE_EXECUTE, DIRECT_THREADED\n",
            emulator->emulation_mode);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int run(int argc, char **argv) {
    config_t config;
    emulator_t emulator;
    emulation_mode_t mode;

    init_default_config(&config);

    const int parse_result = parse_cli_args(argc, argv, &config);
    if (parse_result != -1) {
        return parse_result;
    }

    if (resolve_emulation_mode(&config, &mode) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    const char *trace_env = getenv("EMULATION_TRACE");
    mos6502_trace_enabled = (trace_env != NULL && trace_env[0] != '\0' && trace_env[0] != '0');

    if (initialize_emulator(&emulator, mode) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (initialize_cpu(&emulator, config.program_path) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return run_emulation(&emulator);
}
