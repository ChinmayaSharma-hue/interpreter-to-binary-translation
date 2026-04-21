//
// Created by chinmay on 10/04/26.
//

#ifndef MOS6502_ENCODER_H
#define MOS6502_ENCODER_H
#include <stdbool.h>
#include <stdint.h>

#define MAX_INSTRUCTION_SIZE 15
#define INSTRUCTION_END 0xFF

typedef enum {
    EMIT_OK = 0,
    EMIT_ERR_SIZE_MISMATCH,
    EMIT_ERR_SIZE_INVALID,
    EMIT_ERR_INVALID_REGISTER,
    EMIT_ERR_INVALID_SCALE,
    EMIT_ERR_INVALID_ADDRESSING
} emit_error_t;

typedef struct {
    uint8_t instruction_bytes[MAX_INSTRUCTION_SIZE];
    emit_error_t error;
} emit_result_t;

typedef enum {
    al, ah, ax, eax, rax,
    bl, bh, bx, ebx, rbx,
    cl, ch, cx, ecx, rcx,
    dl, dh, dx, edx, rdx,
    sil, si, esi, rsi,
    dil, di, edi, rdi,
    spl, sp, esp, rsp,
    bpl, bp, ebp, rbp,
    r8b, r8w, r8d, r8,
    r9b, r9w, r9d, r9,
    r10b, r10w, r10d, r10,
    r11b, r11w, r11d, r11,
    r12b, r12w, r12d, r12,
    r13b, r13w, r13d, r13,
    r14b, r14w, r14d, r14,
    r15b, r15w, r15d, r15,
} registers_t;

typedef enum {
    REG_SIZE_BYTE  = 1,
    REG_SIZE_WORD  = 2,
    REG_SIZE_DWORD = 4,
    REG_SIZE_QWORD = 8,
} reg_size_t;

typedef enum {
    BYTE_PTR = 1,
    WORD_PTR = 2,
    DWORD_PTR = 4,
    QWORD_PTR = 8
} mem_ptr_t;

typedef enum {
    REG_CLASS_A,
    REG_CLASS_B,
    REG_CLASS_C,
    REG_CLASS_D,
    REG_CLASS_SI,
    REG_CLASS_DI,
    REG_CLASS_SP,
    REG_CLASS_BP,
    REG_CLASS_R8,
    REG_CLASS_R9,
    REG_CLASS_R10,
    REG_CLASS_R11,
    REG_CLASS_R12,
    REG_CLASS_R13,
    REG_CLASS_R14,
    REG_CLASS_R15,
    REG_CLASS_INVALID
} reg_class_t;

typedef struct {
    reg_class_t reg_class;
    reg_size_t size;
    uint8_t num;
    uint8_t needs_rex;
    uint8_t is_high;
} reg_info_t;

typedef enum {
    ADDR_MODE_INDIRECT,
    ADDR_MODE_INDIRECT_DISP8,
    ADDR_MODE_INDIRECT_DISP32,
    ADDR_MODE_DIRECT,
    ADDR_MODE_DISP32_ONLY
} addressing_mode;

typedef struct {
    uint8_t mod: 2;
    uint8_t reg: 3;
    uint8_t rm: 3;
} modRM_encoded;

typedef struct {
    union {
        reg_info_t reg;
        uint8_t digit;
    };
    reg_info_t rm;
    addressing_mode mode;
    bool has_digit;
} modRM_decoded;

typedef struct {
    uint8_t scale:  2;
    uint8_t index: 3;
    uint8_t base: 3;
} sib_encoded;

typedef struct {
    reg_info_t index;
    reg_info_t base;
    uint8_t scale;
} sib_decoded;


typedef struct {
    bool emit_66h;
    uint8_t opcodes[3];
    uint8_t num_opcodes;
    // registers for REX computation when has_modRM is false
    reg_info_t rex_reg;
    reg_info_t rex_rm;
    modRM_decoded modRM;
    sib_decoded sib;
    uint8_t imm_bytes[8];
    uint8_t imm_len;
    uint8_t disp8;
    uint8_t disp32;
    bool has_modRM;
    bool has_sib;
} emit_descriptor_t;

static inline reg_info_t reg_info(const registers_t reg) {
    switch (reg) {
        // rax family
        case al:   return (reg_info_t){ REG_CLASS_A,   REG_SIZE_BYTE,  0,  0, 0 };
        case ah:   return (reg_info_t){ REG_CLASS_A,   REG_SIZE_BYTE,  4,  0, 1 };
        case ax:   return (reg_info_t){ REG_CLASS_A,   REG_SIZE_WORD,  0,  0, 0 };
        case eax:  return (reg_info_t){ REG_CLASS_A,   REG_SIZE_DWORD, 0,  0, 0 };
        case rax:  return (reg_info_t){ REG_CLASS_A,   REG_SIZE_QWORD, 0,  0, 0 };

        // rbx family
        case bl:   return (reg_info_t){ REG_CLASS_B,   REG_SIZE_BYTE,  3,  0, 0 };
        case bh:   return (reg_info_t){ REG_CLASS_B,   REG_SIZE_BYTE,  7,  0, 1 };
        case bx:   return (reg_info_t){ REG_CLASS_B,   REG_SIZE_WORD,  3,  0, 0 };
        case ebx:  return (reg_info_t){ REG_CLASS_B,   REG_SIZE_DWORD, 3,  0, 0 };
        case rbx:  return (reg_info_t){ REG_CLASS_B,   REG_SIZE_QWORD, 3,  0, 0 };

        // rcx family
        case cl:   return (reg_info_t){ REG_CLASS_C,   REG_SIZE_BYTE,  1,  0, 0 };
        case ch:   return (reg_info_t){ REG_CLASS_C,   REG_SIZE_BYTE,  5,  0, 1 };
        case cx:   return (reg_info_t){ REG_CLASS_C,   REG_SIZE_WORD,  1,  0, 0 };
        case ecx:  return (reg_info_t){ REG_CLASS_C,   REG_SIZE_DWORD, 1,  0, 0 };
        case rcx:  return (reg_info_t){ REG_CLASS_C,   REG_SIZE_QWORD, 1,  0, 0 };

        // rdx family
        case dl:   return (reg_info_t){ REG_CLASS_D,   REG_SIZE_BYTE,  2,  0, 0 };
        case dh:   return (reg_info_t){ REG_CLASS_D,   REG_SIZE_BYTE,  6,  0, 1 };
        case dx:   return (reg_info_t){ REG_CLASS_D,   REG_SIZE_WORD,  2,  0, 0 };
        case edx:  return (reg_info_t){ REG_CLASS_D,   REG_SIZE_DWORD, 2,  0, 0 };
        case rdx:  return (reg_info_t){ REG_CLASS_D,   REG_SIZE_QWORD, 2,  0, 0 };

        // rsi family
        // note: sil needs REX to distinguish from dh (both would encode as num=6
        // without REX — REX presence changes the interpretation)
        case sil:  return (reg_info_t){ REG_CLASS_SI,  REG_SIZE_BYTE,  6,  1, 0 };
        case si:   return (reg_info_t){ REG_CLASS_SI,  REG_SIZE_WORD,  6,  0, 0 };
        case esi:  return (reg_info_t){ REG_CLASS_SI,  REG_SIZE_DWORD, 6,  0, 0 };
        case rsi:  return (reg_info_t){ REG_CLASS_SI,  REG_SIZE_QWORD, 6,  0, 0 };

        // rdi family
        // note: dil needs REX to distinguish from bh (both encode as num=7
        // without REX)
        case dil:  return (reg_info_t){ REG_CLASS_DI,  REG_SIZE_BYTE,  7,  1, 0 };
        case di:   return (reg_info_t){ REG_CLASS_DI,  REG_SIZE_WORD,  7,  0, 0 };
        case edi:  return (reg_info_t){ REG_CLASS_DI,  REG_SIZE_DWORD, 7,  0, 0 };
        case rdi:  return (reg_info_t){ REG_CLASS_DI,  REG_SIZE_QWORD, 7,  0, 0 };

        // rsp family
        // note: spl needs REX, and num=4 as rm triggers SIB byte
        case spl:  return (reg_info_t){ REG_CLASS_SP,  REG_SIZE_BYTE,  4,  1, 0 };
        case sp:   return (reg_info_t){ REG_CLASS_SP,  REG_SIZE_WORD,  4,  0, 0 };
        case esp:  return (reg_info_t){ REG_CLASS_SP,  REG_SIZE_DWORD, 4,  0, 0 };
        case rsp:  return (reg_info_t){ REG_CLASS_SP,  REG_SIZE_QWORD, 4,  0, 0 };

        // rbp family
        // note: bpl needs REX, and num=5 with mod=00 as rm gives RIP-relative
        case bpl:  return (reg_info_t){ REG_CLASS_BP,  REG_SIZE_BYTE,  5,  1, 0 };
        case bp:   return (reg_info_t){ REG_CLASS_BP,  REG_SIZE_WORD,  5,  0, 0 };
        case ebp:  return (reg_info_t){ REG_CLASS_BP,  REG_SIZE_DWORD, 5,  0, 0 };
        case rbp:  return (reg_info_t){ REG_CLASS_BP,  REG_SIZE_QWORD, 5,  0, 0 };

        // r8 family — all need REX (num >= 8, REX.B/R/X extends encoding)
        case r8b:  return (reg_info_t){ REG_CLASS_R8,  REG_SIZE_BYTE,  8,  1, 0 };
        case r8w:  return (reg_info_t){ REG_CLASS_R8,  REG_SIZE_WORD,  8,  1, 0 };
        case r8d:  return (reg_info_t){ REG_CLASS_R8,  REG_SIZE_DWORD, 8,  1, 0 };
        case r8:   return (reg_info_t){ REG_CLASS_R8,  REG_SIZE_QWORD, 8,  1, 0 };

        // r9 family
        case r9b:  return (reg_info_t){ REG_CLASS_R9,  REG_SIZE_BYTE,  9,  1, 0 };
        case r9w:  return (reg_info_t){ REG_CLASS_R9,  REG_SIZE_WORD,  9,  1, 0 };
        case r9d:  return (reg_info_t){ REG_CLASS_R9,  REG_SIZE_DWORD, 9,  1, 0 };
        case r9:   return (reg_info_t){ REG_CLASS_R9,  REG_SIZE_QWORD, 9,  1, 0 };

        // r10 family
        case r10b: return (reg_info_t){ REG_CLASS_R10, REG_SIZE_BYTE,  10, 1, 0 };
        case r10w: return (reg_info_t){ REG_CLASS_R10, REG_SIZE_WORD,  10, 1, 0 };
        case r10d: return (reg_info_t){ REG_CLASS_R10, REG_SIZE_DWORD, 10, 1, 0 };
        case r10:  return (reg_info_t){ REG_CLASS_R10, REG_SIZE_QWORD, 10, 1, 0 };

        // r11 family
        case r11b: return (reg_info_t){ REG_CLASS_R11, REG_SIZE_BYTE,  11, 1, 0 };
        case r11w: return (reg_info_t){ REG_CLASS_R11, REG_SIZE_WORD,  11, 1, 0 };
        case r11d: return (reg_info_t){ REG_CLASS_R11, REG_SIZE_DWORD, 11, 1, 0 };
        case r11:  return (reg_info_t){ REG_CLASS_R11, REG_SIZE_QWORD, 11, 1, 0 };

        // r12 family
        // note: r12 as rm triggers SIB byte (same encoding issue as rsp)
        case r12b: return (reg_info_t){ REG_CLASS_R12, REG_SIZE_BYTE,  12, 1, 0 };
        case r12w: return (reg_info_t){ REG_CLASS_R12, REG_SIZE_WORD,  12, 1, 0 };
        case r12d: return (reg_info_t){ REG_CLASS_R12, REG_SIZE_DWORD, 12, 1, 0 };
        case r12:  return (reg_info_t){ REG_CLASS_R12, REG_SIZE_QWORD, 12, 1, 0 };

        // r13 family
        // note: r13 with mod=00 as rm gives disp32-only (same issue as rbp)
        case r13b: return (reg_info_t){ REG_CLASS_R13, REG_SIZE_BYTE,  13, 1, 0 };
        case r13w: return (reg_info_t){ REG_CLASS_R13, REG_SIZE_WORD,  13, 1, 0 };
        case r13d: return (reg_info_t){ REG_CLASS_R13, REG_SIZE_DWORD, 13, 1, 0 };
        case r13:  return (reg_info_t){ REG_CLASS_R13, REG_SIZE_QWORD, 13, 1, 0 };

        // r14 family
        case r14b: return (reg_info_t){ REG_CLASS_R14, REG_SIZE_BYTE,  14, 1, 0 };
        case r14w: return (reg_info_t){ REG_CLASS_R14, REG_SIZE_WORD,  14, 1, 0 };
        case r14d: return (reg_info_t){ REG_CLASS_R14, REG_SIZE_DWORD, 14, 1, 0 };
        case r14:  return (reg_info_t){ REG_CLASS_R14, REG_SIZE_QWORD, 14, 1, 0 };

        // r15 family
        case r15b: return (reg_info_t){ REG_CLASS_R15, REG_SIZE_BYTE,  15, 1, 0 };
        case r15w: return (reg_info_t){ REG_CLASS_R15, REG_SIZE_WORD,  15, 1, 0 };
        case r15d: return (reg_info_t){ REG_CLASS_R15, REG_SIZE_DWORD, 15, 1, 0 };
        case r15:  return (reg_info_t){ REG_CLASS_R15, REG_SIZE_QWORD, 15, 1, 0 };

        default:   return (reg_info_t){ REG_CLASS_INVALID, 0, 0, 0, 0 };
    }
}

// data movement
emit_result_t emit_mov_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_mov_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_mov_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_mov_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_mov_mem_reg(registers_t destination, registers_t source);
emit_result_t emit_movzx_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_movzx_reg_mem(registers_t destination, registers_t source, mem_ptr_t pointer);

// effective address
emit_result_t emit_lea_sib(
    registers_t destination,
    uint8_t scale,
    registers_t index,
    registers_t base
);
emit_result_t emit_lea_sib_disp8(
    registers_t destination,
    uint8_t scale,
    registers_t index,
    registers_t base,
    uint8_t displacement
);
emit_result_t emit_lea_sib_disp32(
    registers_t destination,
    uint8_t scale,
    registers_t index,
    registers_t base,
    uint32_t displacement
);
emit_result_t emit_lea_reg_disp8(registers_t destination, registers_t source, uint8_t displacement);
emit_result_t emit_lea_reg_disp32(registers_t destination, registers_t source, uint32_t displacement);

// alu operations
emit_result_t emit_add_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_add_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_add_reg_imm16(registers_t destination, uint16_t immediate);
emit_result_t emit_add_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_adc_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_adc_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_adc_reg_imm16(registers_t destination, uint16_t immediate);
emit_result_t emit_adc_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_adc_reg_imm(registers_t destination, uint32_t immediate);
emit_result_t emit_adc_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_sub_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_sub_reg_imm(registers_t destination, uint32_t immediate);
emit_result_t emit_sub_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_cmp_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_cmp_reg_imm(registers_t destination, uint32_t immediate);
emit_result_t emit_cmp_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_and_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_and_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_and_reg_imm16(registers_t destination, uint16_t immediate);
emit_result_t emit_and_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_and_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_or_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_or_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_or_reg_imm16(registers_t destination, uint16_t immediate);
emit_result_t emit_or_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_or_reg_mem(registers_t destination, registers_t source);
emit_result_t emit_xor_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_xor_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_xor_reg_imm16(registers_t destination, uint16_t immediate);
emit_result_t emit_xor_reg_imm32(registers_t destination, uint32_t immediate);
emit_result_t emit_xor_reg_mem(registers_t destination, registers_t source);

// group operations (inc, dec, not, test, shifts, rotates)
emit_result_t emit_inc_reg(registers_t destination);
emit_result_t emit_inc_mem(registers_t destination, mem_ptr_t ptr_size);
emit_result_t emit_dec_reg(registers_t destination);
emit_result_t emit_dec_mem(registers_t destination, mem_ptr_t ptr_size);
emit_result_t emit_not_reg(registers_t destination);
emit_result_t emit_test_reg_reg(registers_t destination, registers_t source);
emit_result_t emit_test_reg_imm8(registers_t destination, uint8_t immediate);
emit_result_t emit_shl_reg_1(registers_t destination);
emit_result_t emit_shl_reg_imm(registers_t destination, uint8_t immediate);
emit_result_t emit_shl_mem_1(registers_t destination, mem_ptr_t ptr_size);
emit_result_t emit_shr_reg_1(registers_t destination);
emit_result_t emit_shr_reg_imm(registers_t destination, uint8_t immediate);
emit_result_t emit_shr_mem_1(registers_t destination, mem_ptr_t ptr_size);
emit_result_t emit_rcl_reg_1(registers_t destination);
emit_result_t emit_rcl_mem_1(registers_t destination, mem_ptr_t ptr_size);
emit_result_t emit_rcr_reg_1(registers_t destination);
emit_result_t emit_rcr_mem_1(registers_t destination, mem_ptr_t ptr_size);

// flag operations
emit_result_t emit_bt_reg_imm(registers_t source, uint8_t bit_index);
emit_result_t emit_setc_reg(registers_t destination);
emit_result_t emit_setnc_reg(registers_t destination);
emit_result_t emit_sete_reg(registers_t destination);
emit_result_t emit_setne_reg(registers_t destination);
emit_result_t emit_sets_reg(registers_t destination);
emit_result_t emit_seto_reg(registers_t destination);
emit_result_t emit_seta_reg(registers_t destination);
emit_result_t emit_setz_reg(registers_t destination);
emit_result_t emit_setnz_reg(registers_t destination);
emit_result_t emit_stc(void);
emit_result_t emit_clc(void);

// miscellaneous
emit_result_t emit_nop(void);
#endif //MOS6502_ENCODER_H