---
layout: default
title: Interpreter for CHIP-8
permalink: /direct-threading-with-dynamic-precoding/
---

### Problem Context
The direct threading with static precoding is invalid when the binary has,
- **Self Modifying Code**, if there are instructions that modify other instructions, then static precoding will lead to invalidated instructions in the code cache leading to incorrect execution.
- **Code discovery challenges**, the static precoding stops when the instruction is invalid, i.e., it encounters data blocks, but there could be instructions beyond the data blocks which have not been precoding.

### Minimal Solution
- **For self modifying code**, add support for invalidating the corresponding precoding in the code cache and dynamically precode the updated instruction during execution to add to the code cache.
- **For data interspersed with code**, add support for dynamically precoding the instructions during execution.
- **To find the set of instructions to dynamically precode during execution**, store instruction starts and their mapping to code cache indexes, to know where to start dynamically precoding from and where to stop.

### Implementation

The modification of execution,

![img_3.png](img_3.png)

1. Adding additional state members in the chip state,
   ```c
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
    ```
2. Adding support for code cache and instruction start mapping invalidation,
   ```c 
   static void invalidate_translation_entry(chip_t* chip, const uint16_t spc) {
        // resetting the translation map
        if (chip->translation_map[spc] != UINT16_MAX) {
            fprintf(stderr,
                "%s (source_pc=%04X, translation_start=%04X, mapped_address=%04X)\n",
                "The translation for this SPC is being invalidated",
                spc,
                spc,
                chip->translation_map[spc]);
            chip->translation_map[spc] = UINT16_MAX;
        }
    }
   
   static void invalidate_instruction_ownership(chip_t* chip, const uint16_t spc) {
       // resetting the instruction start map
       size_t instruction_start_map_index = spc;
       if (chip->instruction_start_map[instruction_start_map_index] != UINT16_MAX) {
           while (instruction_start_map_index < CODE_CACHE_CAPACITY &&
           chip->instruction_start_map[instruction_start_map_index] == spc) {
               chip->instruction_start_map[instruction_start_map_index] = UINT16_MAX;
               instruction_start_map_index++;
           }
       }
   }
   ```
3. Allowing precoding on a more granular level,
    ```c
   static int translate_instruction(chip_t* chip, const decode_entry_t *dispatch, const uint16_t spc, const uint16_t tpc) {
        const uint8_t opcode = chip->memory[spc];

        // validity and bound checks
        if (!valid_opcode[opcode]) {
            invalidate_translation_entry(chip, spc);
            return -1;
        }

        const uint8_t length = instruction_length[opcode];
        if (spc > (uint16_t)(MEMORY_SIZE - length)) {
            invalidate_translation_entry(chip, spc);
            return -1;
        }

        // invalidating the instruction range ownership for this SPC
        invalidate_instruction_ownership(chip, spc);

        // construction of the threaded instruction
        threaded_instructions_t instruction = {
            .handler = dispatch[opcode].handler,
            .mode = dispatch[opcode].mode,
            .spc_byte_offset = length
        };
        chip->instruction_start_map[spc] = spc;
        if (length == 2) {
            instruction.operand = chip->memory[spc+1];
            chip->instruction_start_map[spc+1] = spc;
        } else if (length == 3) {
            instruction.operand = chip->memory[spc+1] | (chip->memory[spc+2] << 8);
            chip->instruction_start_map[spc+1] = spc;
            chip->instruction_start_map[spc+2] = spc;
        }
        chip->code_cache[tpc] = instruction;
        chip->translation_map[spc] = tpc;

        return length;
    } 
    ```
4. Adding support to dynamically precode,
    ```c
   size_t cpu_translate(chip_t* chip, const decode_entry_t *dispatch) {
       uint16_t spc_translate = chip->program_counter;
       while (true) {
           const int length = translate_instruction(chip, dispatch, spc_translate, (uint16_t)chip->cache_length);
           if (length < 0) break;
           // increments
           chip->cache_length += 1;
           spc_translate += length;
           // guard against overflow
           if (chip->cache_length == CODE_CACHE_CAPACITY) break;
       }
       return chip->cache_length;
   } 
    ```
5. Detecting self modifying code,
   ```c
   static inline void detect_self_modifying_code(
        chip_t *chip,
        const uint16_t address,
        const decode_entry_t *dispatch,
        const translate_instruction_fn_t translate_instruction_fn
    ) {
        uint16_t translation_address = chip->instruction_start_map[address];
        if (translation_address == UINT16_MAX) {
            return;
        }

        uint16_t tpc = chip->translation_map[translation_address];
        REPORT_SELF_MODIFYING_CODE(chip, address, translation_address, tpc);

        int length_translated;
        if (tpc != UINT16_MAX) {
            length_translated = translate_instruction_fn(chip, dispatch, translation_address, tpc);
        } else {
            if (chip->cache_length >= CODE_CACHE_CAPACITY) {
                return;
            }
            tpc = (uint16_t)(chip->cache_length);
            length_translated = translate_instruction_fn(chip, dispatch, translation_address, tpc);
            if (length_translated > 0) {
                chip->cache_length += 1;
            }
        }

        if (length_translated <= 0) {
            return;
        }
        translation_address += (uint16_t)length_translated;

        while (!((chip->instruction_start_map[translation_address] == translation_address) &&
                 (chip->translation_map[translation_address] != UINT16_MAX))) {
            tpc = chip->translation_map[translation_address];
            if (tpc != UINT16_MAX) {
                length_translated = translate_instruction_fn(chip, dispatch, translation_address, tpc);
            } else {
                if (chip->cache_length >= CODE_CACHE_CAPACITY) {
                    return;
                }
                tpc = (uint16_t)(chip->cache_length);
                length_translated = translate_instruction_fn(chip, dispatch, translation_address, tpc);
                if (length_translated > 0) {
                    chip->cache_length += 1;
                }
            }

            if (length_translated <= 0) {
                return;
            }
            translation_address += (uint16_t)length_translated;
        }
   }
   ```
   This function precodes the instruction that exists in the address that has been modified, and if the modification interferes with the existing instruction boundaries, then multiple instructions have to be re-precoded until the precoding is valid.
6. If precoded mappings are not found, then dynamic precoding has to take place while proceeding to the next instruction normally or during jumps,
   ```c 
   #define NEXT(chip, instruction)                                                              \
    do {                                                                                        \
        (chip)->program_counter += (instruction).spc_byte_offset;                               \
        uint16_t mapped_address = (chip)->translation_map[(chip)->program_counter];             \
        if (mapped_address == UINT16_MAX) {                                                     \
            size_t previous_cache_length = (chip)->cache_length;                                \
            if (previous_cache_length == cpu_translate(chip, dispatch)) {                       \
                REPORT_TRANSLATION_DOES_NOT_EXIST(chip, mapped_address);                        \
                exit(0);                                                                        \
            }                                                                                   \
        }                                                                                       \
        mapped_address = (chip)->translation_map[(chip)->program_counter];                      \
        (instruction) = (chip)->code_cache[mapped_address];                                     \
        TRACE_CPU_STATE(chip);                                                                  \
        goto *(instruction).handler;                                                            \
    } while (0)

    #define JUMP(chip, instruction, address)                                                    \
    do {                                                                                        \
        (chip)->program_counter = address;                                                      \
        uint16_t mapped_address = (chip)->translation_map[(chip)->program_counter];             \
        if (mapped_address == UINT16_MAX) {                                                     \
            size_t previous_cache_length = (chip)->cache_length;                                \
            if (previous_cache_length == cpu_translate(chip, dispatch)) {                       \
                REPORT_TRANSLATION_DOES_NOT_EXIST(chip, mapped_address);                        \
                exit(0);                                                                        \
            }                                                                                   \
        }                                                                                       \
        mapped_address = (chip)->translation_map[(chip)->program_counter];                      \
        (instruction) = (chip)->code_cache[mapped_address];                                     \
        TRACE_CPU_STATE(chip);                                                                  \
        goto *(instruction).handler;                                                            \
   ```
   The function `cpu_translate` is called which dynamically precodes the next batch of instructions, which couldn't have been statically precoded due to issues with code discovery. 