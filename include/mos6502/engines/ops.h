#ifndef MOS6502_ENGINE_OPS_H
#define MOS6502_ENGINE_OPS_H

#include <stdbool.h>
#include <stdlib.h>

#define LOG_INSTRUCTION_DOES_NOT_EXIST "This instruction does not exist"
#define LOG_TRANSLATION_DOES_NOT_EXIST "The translation for the next instruction does not exist"
#define LOG_TRANSLATION_FAILED "Dynamic translation of instruction failed"
#define LOG_SELF_MODIFYING_CODE_DETECTED "Self modifying code detected"

#define REPORT_INSTRUCTION_DOES_NOT_EXIST(chip)                                             \
    do {                                                                                    \
        switch (emulator->emulation_mode) {                                                 \
            case EMULATION_MODE_DIRECT_THREADED:                                            \
                THREADED_REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);                           \
                break;                                                                      \
            case EMULATION_MODE_BLOCK:                                                      \
                BLOCK_REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);                              \
                break;                                                                      \
            default:                                                                        \
                exit(0);                                                                    \
        }                                                                                   \
    } while (0)

#define REPORT_TRANSLATION_DOES_NOT_EXIST(chip, mapped_address)                             \
    do {                                                                                    \
        fprintf(stderr,                                                                     \
                "%s (source_pc=%04X, mapped_address/mapped_index=%04X)\n",                  \
                LOG_TRANSLATION_DOES_NOT_EXIST,                                             \
                (chip)->program_counter,                                                    \
                (mapped_address));                                                          \
    } while (0)

#if defined(MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE) && MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE
#define REPORT_SELF_MODIFYING_CODE(chip, write_address)                                       \
    do {                                                                                      \
        fprintf(stderr,                                                                       \
                "%s (source_pc=%04X, write_address=%04X)\n",                                  \
                LOG_SELF_MODIFYING_CODE_DETECTED,                                             \
                (chip)->program_counter, write_address                                        \
        );                                                                                    \
    } while (0)
#else
#define REPORT_SELF_MODIFYING_CODE(chip, write_address)                                       \
    do {                                                                                      \
        (void)(chip);                                                                         \
        (void)(write_address);                                                                \
    } while (0)
#endif

#define EA_ABS(chip, instruction) ((uint16_t)((instruction).operand))
#define EA_ABS_X(chip, instruction) ((uint16_t)((instruction).operand + (chip)->index_x_register))
#define EA_ABS_Y(chip, instruction) ((uint16_t)((instruction).operand + (chip)->index_y_register))
#define EA_ZP(chip, instruction) ((uint16_t)((uint8_t)((instruction).operand)))
#define EA_ZP_X(chip, instruction) ((uint16_t)((uint8_t)((instruction).operand + (chip)->index_x_register)))
#define EA_ZP_Y(chip, instruction) ((uint16_t)((uint8_t)((instruction).operand + (chip)->index_y_register)))
#define EA_IND_X(chip, instruction) \
    ((uint16_t)((chip)->memory[(uint8_t)((instruction).operand + (chip)->index_x_register)] | \
    ((chip)->memory[(uint8_t)((uint8_t)((instruction).operand + (chip)->index_x_register) + 1)] << 8)))
#define EA_IND_Y(chip, instruction) \
    ((uint16_t)(((chip)->memory[(uint8_t)((instruction).operand)] | \
    ((chip)->memory[(uint8_t)((uint8_t)((instruction).operand) + 1)] << 8)) + (chip)->index_y_register))
#define VALUE_IMM(instruction) ((uint8_t)((instruction).operand))

#define SET_ZERO_AND_NEGATIVE_FLAGS(chip, value)           \
    do {                                                   \
        if ((value) == 0)                                  \
            (chip)->status_register |= 0x02;               \
        else                                               \
            (chip)->status_register &= (uint8_t)~0x02;     \
        if ((value) & 0x80)                                \
            (chip)->status_register |= 0x80;               \
        else                                               \
            (chip)->status_register &= (uint8_t)~0x80;     \
        (chip)->status_register |= 0x20;                   \
    } while (0)

#define SET_CARRY(chip, condition)              \
    do {                                        \
        if ((condition)) {                      \
            (chip)->status_register |= 0x01;    \
        } else {                                \
            (chip)->status_register &= ~0x01;   \
        }                                       \
    } while (0)

#define SET_OVERFLOW(chip, overflow)            \
    do {                                        \
        if ((overflow)) {                       \
            (chip)->status_register |= 0x40;    \
        } else {                                \
            (chip)->status_register &= ~0x40;   \
        }                                       \
    } while (0)

#define SET_DECIMAL(chip, condition)            \
    do {                                        \
        if ((condition)) {                      \
            (chip)->status_register |= 0x8;     \
        } else {                                \
            (chip)->status_register &= ~0x8;    \
        }                                       \
    } while (0)

#define SET_INTERRUPT_DISABLE(chip, condition)  \
    do {                                        \
        if ((condition)) {                      \
            (chip)->status_register |= 0x4;     \
        } else {                                \
            (chip)->status_register &= ~0x4;    \
        }                                       \
    } while (0)

#define ENGINE_NEXT(chip, instruction)                                                          \
    do {                                                                                        \
        switch (emulator->emulation_mode) {                                                     \
            case EMULATION_MODE_DIRECT_THREADED:                                                \
                THREADED_NEXT((chip), (instruction));                                           \
                break;                                                                          \
            case EMULATION_MODE_BLOCK:                                                          \
                BLOCK_NEXT((chip), (instruction));                                              \
                break;                                                                          \
            default:                                                                            \
                exit(0);                                                                        \
        }                                                                                       \
    } while (0)

#define ENGINE_JUMP(chip, instruction, address)                                                 \
    do {                                                                                        \
        switch (emulator->emulation_mode) {                                                     \
            case EMULATION_MODE_DIRECT_THREADED:                                                \
                THREADED_JUMP((chip), (instruction), (address));                                \
                break;                                                                          \
            case EMULATION_MODE_BLOCK:                                                          \
                BLOCK_JUMP((chip), (instruction), (address));                                   \
                break;                                                                          \
            default:                                                                            \
                exit(0);                                                                        \
        }                                                                                       \
    } while (0)

#define HANDLE_SMC_WRITE(chip, address)                                                        \
    do {                                                                                        \
        switch (emulator->emulation_mode) {                                                     \
            case EMULATION_MODE_DIRECT_THREADED:                                                \
                THREADED_CODE_WRITE((chip), (address));                                         \
                break;                                                                          \
            case EMULATION_MODE_BLOCK:                                                          \
                BLOCK_CODE_WRITE((chip), (address));                                            \
                break;                                                                          \
            default:                                                                            \
                break;                                                                        \
        }                                                                                       \
    } while (0)

#define PUSH_TO_STACK(chip, value)                                      \
    do {                                                                \
        uint16_t address = (uint16_t)(0x0100 | (chip)->stack_pointer);  \
        (chip)->memory[address] = (value);                              \
        HANDLE_SMC_WRITE(chip, address);                               \
        (chip)->stack_pointer--;                                        \
    } while (0)

#define PULL_FROM_STACK(chip) \
    ((chip)->stack_pointer++, (chip)->memory[0x0100 | (chip)->stack_pointer])

#define OP_LOAD_REG(chip, instruction, reg)                              \
    do {                                                                 \
        switch ((instruction).mode) {                                    \
            case AM_IMM:                                                 \
                (reg) = VALUE_IMM(instruction);                          \
                break;                                                   \
            case AM_ZP:                                                  \
                (reg) = (chip)->memory[EA_ZP(chip, instruction)];        \
                break;                                                   \
            case AM_ZP_X:                                                \
                (reg) = (chip)->memory[EA_ZP_X(chip, instruction)];      \
                break;                                                   \
            case AM_ZP_Y:                                                \
                (reg) = (chip)->memory[EA_ZP_Y(chip, instruction)];      \
                break;                                                   \
            case AM_ABS:                                                 \
                (reg) = (chip)->memory[EA_ABS(chip, instruction)];       \
                break;                                                   \
            case AM_ABS_X:                                               \
                (reg) = (chip)->memory[EA_ABS_X(chip, instruction)];     \
                break;                                                   \
            case AM_ABS_Y:                                               \
                (reg) = (chip)->memory[EA_ABS_Y(chip, instruction)];     \
                break;                                                   \
            case AM_IND_X:                                               \
                (reg) = (chip)->memory[EA_IND_X(chip, instruction)];     \
                break;                                                   \
            case AM_IND_Y:                                               \
                (reg) = (chip)->memory[EA_IND_Y(chip, instruction)];     \
                break;                                                   \
            default:                                                     \
                exit(0);                                                 \
        }                                                                \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, (reg));                        \
        ENGINE_NEXT(chip, instruction);                                  \
    } while (0)

#define OP_STORE_REG(chip, instruction, reg)                             \
    do {                                                                 \
        uint16_t address;                                                \
        switch ((instruction).mode) {                                    \
            case AM_ZP:                                                  \
                address = EA_ZP(chip, instruction);                      \
                break;                                                   \
            case AM_ZP_X:                                                \
                address = EA_ZP_X(chip, instruction);                    \
                break;                                                   \
            case AM_ZP_Y:                                                \
                address = EA_ZP_Y(chip, instruction);                    \
                break;                                                   \
            case AM_ABS:                                                 \
                address = EA_ABS(chip, instruction);                     \
                break;                                                   \
            case AM_ABS_X:                                               \
                address = EA_ABS_X(chip, instruction);                   \
                break;                                                   \
            case AM_ABS_Y:                                               \
                address = EA_ABS_Y(chip, instruction);                   \
                break;                                                   \
            case AM_IND_X:                                               \
                address = EA_IND_X(chip, instruction);                   \
                break;                                                   \
            case AM_IND_Y:                                               \
                address = EA_IND_Y(chip, instruction);                   \
                break;                                                   \
            default:                                                     \
                REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);                 \
                exit(0);                                                 \
        }                                                                \
        (chip)->memory[address] = (reg);                                 \
        HANDLE_SMC_WRITE(chip, address);                                \
        ENGINE_NEXT(chip, instruction);                                  \
    } while (0)

#define OP_TRANSFER_TO_REG(reg, value, chip, instruction) \
    do {                                                  \
        (reg) = (value);                                  \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, reg);           \
        ENGINE_NEXT(chip, instruction);                    \
    } while (0)

#define OP_TRANSFER_TO_STACK(reg, chip, instruction) \
    do {                                             \
        (chip)->stack_pointer = (reg);               \
        ENGINE_NEXT(chip, instruction);              \
    } while (0)

#define OP_PUSH_TO_STACK(chip, instruction, value) \
    do {                                           \
        PUSH_TO_STACK(chip, value);                \
        ENGINE_NEXT(chip, instruction);            \
    } while (0)

#define OP_PULL_ACCUMULATOR(chip, instruction)                \
    do {                                                      \
        (chip)->accumulator = PULL_FROM_STACK(chip);          \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, chip->accumulator); \
        ENGINE_NEXT(chip, instruction);                       \
    } while (0)

#define OP_PULL_STATUS_REG(chip, instruction)                  \
    do {                                                       \
        (chip)->status_register = PULL_FROM_STACK(chip);       \
        (chip)->status_register |= 0x30;                       \
        ENGINE_NEXT(chip, instruction);                        \
    } while (0)

#define ASL_MEM(chip, addr)                           \
    do {                                              \
        uint8_t v = (chip)->memory[(addr)];           \
        SET_CARRY(chip, v & 0x80);                    \
        v <<= 1;                                      \
        (chip)->memory[(addr)] = v;                   \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);         \
    } while (0)

#define ASL_ACC(chip)                                 \
    do {                                              \
        uint8_t v = (chip)->accumulator;              \
        SET_CARRY(chip, v & 0x80);                    \
        v <<= 1;                                      \
        (chip)->accumulator = v;                      \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);         \
    } while (0)

#define OP_ASL(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ACC:                                           \
                ASL_ACC(chip);                                     \
                break;                                             \
            case AM_ZP:                                            \
                address = EA_ZP(chip, instruction);                \
                ASL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ZP_X:                                          \
                address = EA_ZP_X(chip, instruction);              \
                ASL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS:                                           \
                address = EA_ABS(chip, instruction);               \
                ASL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS_X:                                         \
                address = EA_ABS_X(chip, instruction);             \
                ASL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            default:                                               \
                REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);           \
                exit(0);                                           \
        }                                                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define LSR_MEM(chip, addr)                                \
    do {                                                   \
        uint8_t v = (chip)->memory[(addr)];                \
        SET_CARRY(chip, v & 0x01);                         \
        v >>= 1;                                           \
        (chip)->memory[(addr)] = v;                        \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);              \
        (chip)->status_register &= (uint8_t)~0x80;         \
    } while (0)

#define LSR_ACC(chip)                                      \
    do {                                                   \
        uint8_t v = (chip)->accumulator;                   \
        SET_CARRY(chip, v & 0x01);                         \
        v >>= 1;                                           \
        (chip)->accumulator = v;                           \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);              \
        (chip)->status_register &= (uint8_t)~0x80;         \
    } while (0)

#define OP_LSR(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ACC:                                           \
                LSR_ACC(chip);                                     \
                break;                                             \
            case AM_ZP:                                            \
                address = EA_ZP(chip, instruction);                \
                LSR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ZP_X:                                          \
                address = EA_ZP_X(chip, instruction);              \
                LSR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS:                                           \
                address = EA_ABS(chip, instruction);               \
                LSR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS_X:                                         \
                address = EA_ABS_X(chip, instruction);             \
                LSR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            default:                                               \
                REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);           \
                exit(0);                                           \
        }                                                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define ROL_MEM(chip, addr)                                 \
    do {                                                    \
        uint8_t v = (chip)->memory[(addr)];                 \
        uint8_t carry = (chip)->status_register & 0x01;     \
        SET_CARRY(chip, v & 0x80);                          \
        v = (uint8_t)((v << 1) | carry);                    \
        (chip)->memory[(addr)] = v;                         \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);               \
    } while (0)

#define ROL_ACC(chip)                                       \
    do {                                                    \
        uint8_t v = (chip)->accumulator;                    \
        uint8_t carry = (chip)->status_register & 0x01;     \
        SET_CARRY(chip, v & 0x80);                          \
        v = (uint8_t)((v << 1) | carry);                    \
        (chip)->accumulator = v;                            \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);               \
    } while (0)

#define OP_ROL(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ACC:                                           \
                ROL_ACC(chip);                                     \
                break;                                             \
            case AM_ZP:                                            \
                address = EA_ZP(chip, instruction);                \
                ROL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ZP_X:                                          \
                address = EA_ZP_X(chip, instruction);              \
                ROL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS:                                           \
                address = EA_ABS(chip, instruction);               \
                ROL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS_X:                                         \
                address = EA_ABS_X(chip, instruction);             \
                ROL_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            default:                                               \
                REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);           \
                exit(0);                                           \
        }                                                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define ROR_MEM(chip, addr)                               \
    do {                                                  \
        uint8_t v = (chip)->memory[(addr)];               \
        uint8_t carry_in =                                \
            (uint8_t)((chip)->status_register & 0x01) << 7; \
        SET_CARRY(chip, v & 0x01);                        \
        v = (uint8_t)((v >> 1) | carry_in);               \
        (chip)->memory[(addr)] = v;                       \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);             \
    } while (0)

#define ROR_ACC(chip)                                      \
    do {                                                   \
        uint8_t v = (chip)->accumulator;                   \
        uint8_t carry_in =                                 \
            (uint8_t)((chip)->status_register & 0x01) << 7; \
        SET_CARRY(chip, v & 0x01);                         \
        v = (uint8_t)((v >> 1) | carry_in);                \
        (chip)->accumulator = v;                           \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, v);              \
    } while (0)

#define OP_ROR(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ACC:                                           \
                ROR_ACC(chip);                                     \
                break;                                             \
            case AM_ZP:                                            \
                address = EA_ZP(chip, instruction);                \
                ROR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ZP_X:                                          \
                address = EA_ZP_X(chip, instruction);              \
                ROR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS:                                           \
                address = EA_ABS(chip, instruction);               \
                ROR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            case AM_ABS_X:                                         \
                address = EA_ABS_X(chip, instruction);             \
                ROR_MEM(chip, address);                            \
                HANDLE_SMC_WRITE(chip, address);                  \
                break;                                             \
            default:                                               \
                REPORT_INSTRUCTION_DOES_NOT_EXIST(chip);           \
                exit(0);                                           \
        }                                                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define AND_VALUE(chip, value) \
    do { (chip)->accumulator &= (value); SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->accumulator); } while (0)

#define OP_AND(chip, instruction)                                                          \
    do {                                                                                   \
        switch ((instruction).mode) {                                                      \
            case AM_IMM: AND_VALUE(chip, VALUE_IMM(instruction)); break;                   \
            case AM_ZP: AND_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: AND_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: AND_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: AND_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: AND_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: AND_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: AND_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0);                     \
        }                                                                                  \
        ENGINE_NEXT(chip, instruction);                                                    \
    } while (0)

#define EOR_VALUE(chip, value) \
    do { (chip)->accumulator ^= (value); SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->accumulator); } while (0)

#define OP_EOR(chip, instruction)                                                          \
    do {                                                                                   \
        switch ((instruction).mode) {                                                      \
            case AM_IMM: EOR_VALUE(chip, VALUE_IMM(instruction)); break;                   \
            case AM_ZP: EOR_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: EOR_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: EOR_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: EOR_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: EOR_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: EOR_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: EOR_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0);                     \
        }                                                                                  \
        ENGINE_NEXT(chip, instruction);                                                    \
    } while (0)

#define BIT_VALUE(chip, value)                                                         \
    do {                                                                               \
        if (((chip)->accumulator & (value)) == 0) (chip)->status_register |= 0x02;    \
        else (chip)->status_register &= (uint8_t)~0x02;                                \
        if ((value) & 0x80) (chip)->status_register |= 0x80;                           \
        else (chip)->status_register &= (uint8_t)~0x80;                                \
        if ((value) & 0x40) (chip)->status_register |= 0x40;                           \
        else (chip)->status_register &= (uint8_t)~0x40;                                \
        (chip)->status_register |= 0x20;                                               \
    } while (0)

#define OP_BIT(chip, instruction)                                                           \
    do {                                                                                    \
        switch ((instruction).mode) {                                                       \
            case AM_ZP: BIT_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break;  \
            case AM_ABS: BIT_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0);                      \
        }                                                                                   \
        ENGINE_NEXT(chip, instruction);                                                     \
    } while (0)

#define ORA_VALUE(chip, value) \
    do { (chip)->accumulator |= (value); SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->accumulator); } while (0)

#define OP_ORA(chip, instruction)                                                          \
    do {                                                                                   \
        switch ((instruction).mode) {                                                      \
            case AM_IMM: ORA_VALUE(chip, VALUE_IMM(instruction)); break;                   \
            case AM_ZP: ORA_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: ORA_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: ORA_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: ORA_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: ORA_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: ORA_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: ORA_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0);                     \
        }                                                                                  \
        ENGINE_NEXT(chip, instruction);                                                    \
    } while (0)

#define ADC_VALUE(chip, value)                                                       \
    do {                                                                             \
        uint8_t a = (chip)->accumulator;                                             \
        uint8_t c = (chip)->status_register & 0x01;                                  \
        uint16_t binary_sum = (uint16_t)a + (uint16_t)(value) + c;                   \
        uint8_t overflow_source;                                                     \
        if ((chip)->status_register & 0x08) {                                        \
            uint8_t low_nibble = (a & 0x0F) + ((value) & 0x0F) + c;                  \
            uint8_t half_carry = (low_nibble > 9) ? 1 : 0;                           \
            uint8_t high_nibble = ((a >> 4) & 0x0F) + ((value >> 4) & 0x0F) + half_carry; \
            uint8_t alu_result = (uint8_t)(((high_nibble & 0x0F) << 4) | ((low_nibble & 0x0F) & 0x0F)); \
            if (low_nibble > 9) low_nibble += 0x06;                                  \
            if (high_nibble > 0x9) high_nibble += 0x06;                              \
            uint8_t bcd_sum = (uint8_t)(((high_nibble & 0x0F) << 4) | ((low_nibble & 0x0F) & 0x0F)); \
            (chip)->accumulator = (uint8_t)(bcd_sum & 0xFF);                         \
            SET_ZERO_AND_NEGATIVE_FLAGS(chip, alu_result);                           \
            SET_CARRY(chip, high_nibble > 0x0F);                                     \
            overflow_source = alu_result;                                            \
        } else {                                                                     \
            (chip)->accumulator = (uint8_t)(binary_sum & 0xFF);                      \
            SET_ZERO_AND_NEGATIVE_FLAGS(chip, (uint8_t)binary_sum);                  \
            SET_CARRY(chip, binary_sum > 0xFF);                                      \
            overflow_source = (uint8_t)binary_sum;                                   \
        }                                                                            \
        uint8_t overflow = (uint8_t)(~(a ^ (value)) & (a ^ overflow_source) & 0x80); \
        SET_OVERFLOW(chip, overflow);                                                \
    } while (0)

#define OP_ADC(chip, instruction) \
    do { \
        switch ((instruction).mode) { \
            case AM_IMM: ADC_VALUE(chip, VALUE_IMM(instruction)); break; \
            case AM_ZP: ADC_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: ADC_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: ADC_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: ADC_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: ADC_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: ADC_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: ADC_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        } \
        ENGINE_NEXT(chip, instruction); \
    } while (0)

#define SBC_VALUE(chip, value)                                                      \
    do {                                                                            \
        uint8_t a = (chip)->accumulator;                                            \
        uint8_t c = (chip)->status_register & 0x01;                                 \
        uint16_t difference = (uint16_t)a + (uint16_t)((uint8_t)~(value)) + c;     \
        uint8_t overflow = (uint8_t)(((a ^ (uint8_t)difference) & (a ^ (value))) & 0x80); \
        uint8_t binary_difference = (uint8_t)difference;                            \
        SET_CARRY(chip, difference > 0xFF);                                         \
        if ((chip)->status_register & 0x08) {                                       \
            if ((a & 0x0F) < (((value) & 0x0F) + (1 - c))) difference -= 0x06;      \
            if (difference < 0x100) difference -= 0x60;                             \
        }                                                                           \
        (chip)->accumulator = (uint8_t)(difference & 0xFF);                         \
        SET_ZERO_AND_NEGATIVE_FLAGS(chip, binary_difference);                       \
        SET_OVERFLOW(chip, overflow);                                               \
    } while (0)

#define OP_SBC(chip, instruction) \
    do { \
        switch ((instruction).mode) { \
            case AM_IMM: SBC_VALUE(chip, VALUE_IMM(instruction)); break; \
            case AM_ZP: SBC_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: SBC_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: SBC_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: SBC_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: SBC_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: SBC_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: SBC_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        } \
        ENGINE_NEXT(chip, instruction); \
    } while (0)

#define CMP_VALUE(chip, value) \
    do { uint8_t a = (chip)->accumulator; uint16_t diff = (uint16_t)a - (uint16_t)(value); SET_ZERO_AND_NEGATIVE_FLAGS(chip, (uint8_t)diff); SET_CARRY(chip, a >= (value)); } while (0)

#define OP_CMP(chip, instruction) \
    do { \
        switch ((instruction).mode) { \
            case AM_IMM: CMP_VALUE(chip, VALUE_IMM(instruction)); break; \
            case AM_ZP: CMP_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ZP_X: CMP_VALUE(chip, (chip)->memory[EA_ZP_X(chip, instruction)]); break; \
            case AM_ABS: CMP_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            case AM_ABS_X: CMP_VALUE(chip, (chip)->memory[EA_ABS_X(chip, instruction)]); break; \
            case AM_ABS_Y: CMP_VALUE(chip, (chip)->memory[EA_ABS_Y(chip, instruction)]); break; \
            case AM_IND_X: CMP_VALUE(chip, (chip)->memory[EA_IND_X(chip, instruction)]); break; \
            case AM_IND_Y: CMP_VALUE(chip, (chip)->memory[EA_IND_Y(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        } \
        ENGINE_NEXT(chip, instruction); \
    } while (0)

#define CPX_VALUE(chip, value) \
    do { uint8_t x = (chip)->index_x_register; uint8_t diff = (uint8_t)(x - (value)); SET_ZERO_AND_NEGATIVE_FLAGS(chip, diff); SET_CARRY(chip, x >= (value)); } while (0)

#define OP_CPX(chip, instruction) \
    do { \
        switch ((instruction).mode) { \
            case AM_IMM: CPX_VALUE(chip, VALUE_IMM(instruction)); break; \
            case AM_ZP: CPX_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ABS: CPX_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        } \
        ENGINE_NEXT(chip, instruction); \
    } while (0)

#define CPY_VALUE(chip, value) \
    do { uint8_t y = (chip)->index_y_register; uint8_t diff = (uint8_t)(y - (value)); SET_ZERO_AND_NEGATIVE_FLAGS(chip, diff); SET_CARRY(chip, y >= (value)); } while (0)

#define OP_CPY(chip, instruction) \
    do { \
        switch ((instruction).mode) { \
            case AM_IMM: CPY_VALUE(chip, VALUE_IMM(instruction)); break; \
            case AM_ZP: CPY_VALUE(chip, (chip)->memory[EA_ZP(chip, instruction)]); break; \
            case AM_ABS: CPY_VALUE(chip, (chip)->memory[EA_ABS(chip, instruction)]); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        } \
        ENGINE_NEXT(chip, instruction); \
    } while (0)

#define DEC_MEM(chip, addr) \
    do { uint8_t v = --((chip)->memory[(addr)]); SET_ZERO_AND_NEGATIVE_FLAGS(chip, v); } while (0)

#define OP_DEC(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ZP: address = EA_ZP(chip, instruction); break; \
            case AM_ZP_X: address = EA_ZP_X(chip, instruction); break; \
            case AM_ABS: address = EA_ABS(chip, instruction); break; \
            case AM_ABS_X: address = EA_ABS_X(chip, instruction); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        }                                                          \
        DEC_MEM(chip, address);                                    \
        HANDLE_SMC_WRITE(chip, address);                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define OP_DEX(chip, instruction) \
    do { (chip)->index_x_register--; SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->index_x_register); ENGINE_NEXT(chip, instruction); } while (0)

#define OP_DEY(chip, instruction) \
    do { (chip)->index_y_register--; SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->index_y_register); ENGINE_NEXT(chip, instruction); } while (0)

#define INC_MEM(chip, addr) \
    do { uint8_t v = ++((chip)->memory[(addr)]); SET_ZERO_AND_NEGATIVE_FLAGS(chip, v); } while (0)

#define OP_INC(chip, instruction)                                  \
    do {                                                           \
        uint16_t address;                                          \
        switch ((instruction).mode) {                              \
            case AM_ZP: address = EA_ZP(chip, instruction); break; \
            case AM_ZP_X: address = EA_ZP_X(chip, instruction); break; \
            case AM_ABS: address = EA_ABS(chip, instruction); break; \
            case AM_ABS_X: address = EA_ABS_X(chip, instruction); break; \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0); \
        }                                                          \
        INC_MEM(chip, address);                                    \
        HANDLE_SMC_WRITE(chip, address);                          \
        ENGINE_NEXT(chip, instruction);                            \
    } while (0)

#define OP_INX(chip, instruction) \
    do { (chip)->index_x_register++; SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->index_x_register); ENGINE_NEXT(chip, instruction); } while (0)

#define OP_INY(chip, instruction) \
    do { (chip)->index_y_register++; SET_ZERO_AND_NEGATIVE_FLAGS(chip, (chip)->index_y_register); ENGINE_NEXT(chip, instruction); } while (0)

#define OP_BCC(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if (!((chip)->status_register & 0x1)) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BCS(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if ((chip)->status_register & 0x1) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BEQ(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if ((chip)->status_register & 0x2) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BMI(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if ((chip)->status_register & 0x80) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BNE(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if (!((chip)->status_register & 0x2)) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BPL(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if (!((chip)->status_register & 0x80)) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BVC(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if (!((chip)->status_register & 0x40)) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)
#define OP_BVS(chip, instruction) \
    do { int8_t offset = (int8_t)(instruction).operand; if ((chip)->status_register & 0x40) (chip)->program_counter += offset; ENGINE_NEXT(chip, instruction); } while (0)

#define OP_JUMP(chip, instruction)                                                            \
    do {                                                                                      \
        uint16_t pc_before = (chip)->program_counter;                                         \
        uint16_t address;                                                                     \
        switch ((instruction).mode) {                                                         \
            case AM_ABS:                                                                      \
                address = EA_ABS(chip, instruction);                                          \
                if (address == pc_before) exit(0);                                            \
                break;                                                                        \
            case AM_IND: {                                                                    \
                uint16_t pointer = EA_ABS(chip, instruction);                                 \
                uint8_t lo = (chip)->memory[pointer];                                         \
                uint8_t hi = (chip)->memory[(pointer & 0xFF00) | ((pointer + 1) & 0x00FF)];  \
                address = lo | (hi << 8);                                                     \
                break;                                                                        \
            }                                                                                 \
            default: REPORT_INSTRUCTION_DOES_NOT_EXIST(chip); exit(0);                        \
        }                                                                                    \
        ENGINE_JUMP(chip, instruction, address);                                              \
    } while (0)

#define OP_JSR(chip, instruction)                                                             \
    do {                                                                                      \
        uint16_t jump_address = EA_ABS(chip, instruction);                                    \
        uint16_t return_address = (chip)->program_counter + (instruction).spc_byte_offset - 1; \
        PUSH_TO_STACK(chip, return_address >> 8);                                             \
        PUSH_TO_STACK(chip, return_address & 0xFF);                                           \
        ENGINE_JUMP(chip, instruction, jump_address);                                         \
    } while (0)

#define OP_RTS(chip, instruction)                                                             \
    do {                                                                                      \
        uint8_t lo = PULL_FROM_STACK(chip);                                                   \
        uint8_t hi = PULL_FROM_STACK(chip);                                                   \
        uint16_t return_address = ((uint16_t)lo | ((uint16_t)hi << 8)) + 1;                  \
        ENGINE_JUMP(chip, instruction, return_address);                                       \
    } while (0)

#define OP_RTI(chip, instruction)                                                             \
    do {                                                                                      \
        (chip)->status_register = PULL_FROM_STACK(chip) | 0x30;                               \
        uint8_t lo = PULL_FROM_STACK(chip);                                                   \
        uint8_t hi = PULL_FROM_STACK(chip);                                                   \
        uint16_t return_address = ((uint16_t)lo | ((uint16_t)hi << 8));                       \
        ENGINE_JUMP(chip, instruction, return_address);                                       \
    } while (0)

#define OP_CLC(chip, instruction) do { SET_CARRY(chip, false); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_CLD(chip, instruction) do { SET_DECIMAL(chip, false); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_CLI(chip, instruction) do { SET_INTERRUPT_DISABLE(chip, false); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_CLV(chip, instruction) do { SET_OVERFLOW(chip, false); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_SEC(chip, instruction) do { SET_CARRY(chip, true); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_SED(chip, instruction) do { SET_DECIMAL(chip, true); ENGINE_NEXT(chip, instruction); } while (0)
#define OP_SEI(chip, instruction) do { SET_INTERRUPT_DISABLE(chip, true); ENGINE_NEXT(chip, instruction); } while (0)

#define OP_BRK(chip, instruction)                                                             \
    do {                                                                                      \
        uint16_t return_address = (chip)->program_counter + (instruction).spc_byte_offset + 1; \
        PUSH_TO_STACK(chip, return_address >> 8);                                             \
        PUSH_TO_STACK(chip, return_address & 0xFF);                                           \
        PUSH_TO_STACK(chip, (chip)->status_register | 0x30);                                  \
        (chip)->status_register |= 0x04;                                                      \
        ENGINE_JUMP(chip, instruction, (chip)->memory[0xFFFE] | ((chip)->memory[0xFFFF] << 8)); \
    } while (0)

#define DECODE_TABLE_CONTENT \
\
/* load to accumulator */ \
[0xA9] = { &&OP_LDA, AM_IMM }, \
[0xA5] = { &&OP_LDA, AM_ZP }, \
[0xB5] = { &&OP_LDA, AM_ZP_X }, \
[0xAD] = { &&OP_LDA, AM_ABS }, \
[0xBD] = { &&OP_LDA, AM_ABS_X }, \
[0xB9] = { &&OP_LDA, AM_ABS_Y }, \
[0xA1] = { &&OP_LDA, AM_IND_X }, \
[0xB1] = { &&OP_LDA, AM_IND_Y }, \
\
/* load to index x */ \
[0xA2] = { &&OP_LDX, AM_IMM }, \
[0xA6] = { &&OP_LDX, AM_ZP }, \
[0xB6] = { &&OP_LDX, AM_ZP_Y }, \
[0xAE] = { &&OP_LDX, AM_ABS }, \
[0xBE] = { &&OP_LDX, AM_ABS_Y }, \
\
/* load to index y */ \
[0xA0] = { &&OP_LDY, AM_IMM }, \
[0xA4] = { &&OP_LDY, AM_ZP }, \
[0xB4] = { &&OP_LDY, AM_ZP_X }, \
[0xAC] = { &&OP_LDY, AM_ABS }, \
[0xBC] = { &&OP_LDY, AM_ABS_X }, \
\
/* store accumulator */ \
[0x85] = { &&OP_STA, AM_ZP }, \
[0x95] = { &&OP_STA, AM_ZP_X }, \
[0x8D] = { &&OP_STA, AM_ABS }, \
[0x9D] = { &&OP_STA, AM_ABS_X }, \
[0x99] = { &&OP_STA, AM_ABS_Y }, \
[0x81] = { &&OP_STA, AM_IND_X }, \
[0x91] = { &&OP_STA, AM_IND_Y }, \
\
/* store index x */ \
[0x86] = { &&OP_STX, AM_ZP }, \
[0x96] = { &&OP_STX, AM_ZP_Y }, \
[0x8E] = { &&OP_STX, AM_ABS }, \
\
/* store index y */ \
[0x84] = { &&OP_STY, AM_ZP }, \
[0x94] = { &&OP_STY, AM_ZP_X }, \
[0x8C] = { &&OP_STY, AM_ABS }, \
\
/* add with carry */ \
[0x69] = { &&OP_ADC, AM_IMM }, \
[0x65] = { &&OP_ADC, AM_ZP }, \
[0x75] = { &&OP_ADC, AM_ZP_X }, \
[0x6D] = { &&OP_ADC, AM_ABS }, \
[0x7D] = { &&OP_ADC, AM_ABS_X }, \
[0x79] = { &&OP_ADC, AM_ABS_Y }, \
[0x61] = { &&OP_ADC, AM_IND_X }, \
[0x71] = { &&OP_ADC, AM_IND_Y }, \
\
/* subtract with borrow */ \
[0xE9] = { &&OP_SBC, AM_IMM }, \
[0xE5] = { &&OP_SBC, AM_ZP }, \
[0xF5] = { &&OP_SBC, AM_ZP_X }, \
[0xED] = { &&OP_SBC, AM_ABS }, \
[0xFD] = { &&OP_SBC, AM_ABS_X }, \
[0xF9] = { &&OP_SBC, AM_ABS_Y }, \
[0xE1] = { &&OP_SBC, AM_IND_X }, \
[0xF1] = { &&OP_SBC, AM_IND_Y }, \
\
/* bitwise and */ \
[0x29] = { &&OP_AND, AM_IMM }, \
[0x25] = { &&OP_AND, AM_ZP }, \
[0x35] = { &&OP_AND, AM_ZP_X }, \
[0x2D] = { &&OP_AND, AM_ABS }, \
[0x3D] = { &&OP_AND, AM_ABS_X }, \
[0x39] = { &&OP_AND, AM_ABS_Y }, \
[0x21] = { &&OP_AND, AM_IND_X }, \
[0x31] = { &&OP_AND, AM_IND_Y }, \
\
/* bit test */ \
[0x24] = { &&OP_BIT, AM_ZP }, \
[0x2C] = { &&OP_BIT, AM_ABS }, \
\
/* bitwise or */ \
[0x09] = { &&OP_ORA, AM_IMM }, \
[0x05] = { &&OP_ORA, AM_ZP }, \
[0x15] = { &&OP_ORA, AM_ZP_X }, \
[0x0D] = { &&OP_ORA, AM_ABS }, \
[0x1D] = { &&OP_ORA, AM_ABS_X }, \
[0x19] = { &&OP_ORA, AM_ABS_Y }, \
[0x01] = { &&OP_ORA, AM_IND_X }, \
[0x11] = { &&OP_ORA, AM_IND_Y }, \
\
/* bitwise exclusive or */ \
[0x49] = { &&OP_EOR, AM_IMM }, \
[0x45] = { &&OP_EOR, AM_ZP }, \
[0x55] = { &&OP_EOR, AM_ZP_X }, \
[0x4D] = { &&OP_EOR, AM_ABS }, \
[0x5D] = { &&OP_EOR, AM_ABS_X }, \
[0x59] = { &&OP_EOR, AM_ABS_Y }, \
[0x41] = { &&OP_EOR, AM_IND_X }, \
[0x51] = { &&OP_EOR, AM_IND_Y }, \
\
/* compare accumulator */ \
[0xC9] = { &&OP_CMP, AM_IMM }, \
[0xC5] = { &&OP_CMP, AM_ZP }, \
[0xD5] = { &&OP_CMP, AM_ZP_X }, \
[0xCD] = { &&OP_CMP, AM_ABS }, \
[0xDD] = { &&OP_CMP, AM_ABS_X }, \
[0xD9] = { &&OP_CMP, AM_ABS_Y }, \
[0xC1] = { &&OP_CMP, AM_IND_X }, \
[0xD1] = { &&OP_CMP, AM_IND_Y }, \
\
/* compare index x */ \
[0xE0] = { &&OP_CPX, AM_IMM }, \
[0xE4] = { &&OP_CPX, AM_ZP }, \
[0xEC] = { &&OP_CPX, AM_ABS }, \
\
/* compare index y */ \
[0xC0] = { &&OP_CPY, AM_IMM }, \
[0xC4] = { &&OP_CPY, AM_ZP }, \
[0xCC] = { &&OP_CPY, AM_ABS }, \
\
/* shift left */ \
[0x0A] = { &&OP_ASL, AM_ACC }, \
[0x06] = { &&OP_ASL, AM_ZP }, \
[0x16] = { &&OP_ASL, AM_ZP_X }, \
[0x0E] = { &&OP_ASL, AM_ABS }, \
[0x1E] = { &&OP_ASL, AM_ABS_X }, \
\
/* shift right */ \
[0x4A] = { &&OP_LSR, AM_ACC }, \
[0x46] = { &&OP_LSR, AM_ZP }, \
[0x56] = { &&OP_LSR, AM_ZP_X }, \
[0x4E] = { &&OP_LSR, AM_ABS }, \
[0x5E] = { &&OP_LSR, AM_ABS_X }, \
\
/* rotate left */ \
[0x2A] = { &&OP_ROL, AM_ACC }, \
[0x26] = { &&OP_ROL, AM_ZP }, \
[0x36] = { &&OP_ROL, AM_ZP_X }, \
[0x2E] = { &&OP_ROL, AM_ABS }, \
[0x3E] = { &&OP_ROL, AM_ABS_X }, \
\
/* rotate right */ \
[0x6A] = { &&OP_ROR, AM_ACC }, \
[0x66] = { &&OP_ROR, AM_ZP }, \
[0x76] = { &&OP_ROR, AM_ZP_X }, \
[0x6E] = { &&OP_ROR, AM_ABS }, \
[0x7E] = { &&OP_ROR, AM_ABS_X }, \
\
/* increment memory */ \
[0xE6] = { &&OP_INC, AM_ZP }, \
[0xF6] = { &&OP_INC, AM_ZP_X }, \
[0xEE] = { &&OP_INC, AM_ABS }, \
[0xFE] = { &&OP_INC, AM_ABS_X }, \
\
/* decrement memory */ \
[0xC6] = { &&OP_DEC, AM_ZP }, \
[0xD6] = { &&OP_DEC, AM_ZP_X }, \
[0xCE] = { &&OP_DEC, AM_ABS }, \
[0xDE] = { &&OP_DEC, AM_ABS_X }, \
\
/* increment register */ \
[0xE8] = { &&OP_INX, AM_IMP }, \
[0xC8] = { &&OP_INY, AM_IMP }, \
\
/* decrement register */ \
[0xCA] = { &&OP_DEX, AM_IMP }, \
[0x88] = { &&OP_DEY, AM_IMP }, \
\
/* conditional branch relative */ \
[0x90] = { &&OP_BCC, AM_REL }, \
[0xB0] = { &&OP_BCS, AM_REL }, \
[0xF0] = { &&OP_BEQ, AM_REL }, \
[0x30] = { &&OP_BMI, AM_REL }, \
[0xD0] = { &&OP_BNE, AM_REL }, \
[0x10] = { &&OP_BPL, AM_REL }, \
[0x50] = { &&OP_BVC, AM_REL }, \
[0x70] = { &&OP_BVS, AM_REL }, \
\
/* unconditional jump */ \
[0x4C] = { &&OP_JMP, AM_ABS }, \
[0x6C] = { &&OP_JMP, AM_IND }, \
\
/* subroutine control */ \
[0x20] = { &&OP_JSR, AM_ABS }, \
[0x60] = { &&OP_RTS, AM_IMP }, \
[0x40] = { &&OP_RTI, AM_IMP }, \
\
/* status flag manipulation */ \
[0x18] = { &&OP_CLC, AM_IMP }, \
[0xD8] = { &&OP_CLD, AM_IMP }, \
[0x58] = { &&OP_CLI, AM_IMP }, \
[0xB8] = { &&OP_CLV, AM_IMP }, \
[0x38] = { &&OP_SEC, AM_IMP }, \
[0xF8] = { &&OP_SED, AM_IMP }, \
[0x78] = { &&OP_SEI, AM_IMP }, \
\
/* register transfers */ \
[0xAA] = { &&OP_TAX, AM_IMP }, \
[0xA8] = { &&OP_TAY, AM_IMP }, \
[0x8A] = { &&OP_TXA, AM_IMP }, \
[0x98] = { &&OP_TYA, AM_IMP }, \
[0xBA] = { &&OP_TSX, AM_IMP }, \
[0x9A] = { &&OP_TXS, AM_IMP }, \
\
/* stack operations */ \
[0x48] = { &&OP_PHA, AM_IMP }, \
[0x08] = { &&OP_PHP, AM_IMP }, \
[0x68] = { &&OP_PLA, AM_IMP }, \
[0x28] = { &&OP_PLP, AM_IMP }, \
\
/* break and no operation */ \
[0x00] = { &&OP_BRK, AM_IMP }, \
[0xEA] = { &&OP_NOP, AM_IMP }

#endif
