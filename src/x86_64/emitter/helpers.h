//
// Created by chinmay on 21/04/26.
//

#ifndef MOS6502_HELPERS_H
#define MOS6502_HELPERS_H
#include "x86_64/emitter.h"

// the questions to ask in each function,
// 1. what are the operand sizes, and how does it determine the prefix byte?
// 2. what is the kind of operation being performed, and how does it determine the opcode byte(s)?
// 3. what kind of addressing mode is used, and how does it determine the modRM byte?
// 4. what kind of addressing mode is used, and how does it determine the SIB byte?
// 4. what kind of addressing mode is used, and how does it determine the displacement byte?
// 5. what kinds of operands are being used, and how does it determine the immediate byte?

static inline uint8_t rex_value(
    const reg_info_t reg_field,
    const reg_info_t rm_field,
    const reg_info_t index_field,
    const reg_info_t base_field
) {
    const reg_size_t max_size = (reg_field.size > rm_field.size)
                                ? reg_field.size
                                : rm_field.size;
    const uint8_t W = (max_size == REG_SIZE_QWORD) ? 1 : 0;
    const uint8_t R = (reg_field.num >= 8)   ? 1 : 0;
    const uint8_t X = (index_field.num >= 8) ? 1 : 0;
    const uint8_t B = (base_field.num >= 8) || (rm_field.num >= 8) ? 1 : 0;

    const uint8_t needed = W || R || X || B ||
                            reg_field.needs_rex ||
                                rm_field.needs_rex ||
                                    index_field.needs_rex ||
                                        base_field.needs_rex;

    const uint8_t rex = 0x40 | (W << 3) | (R << 2) | (X << 1) | B;
    return needed ? rex : 0;
}

// modRM needs to know the following,
// 1. the reg value, which is the straightforward register
// 2. the rm_reg value, which is the register that is,
//    - either going to be used directly (register direct)
//    - or going to be used to compute an effective address (register indirect)
// 3. if sib is going to be used
// 4. if displacement is going to be used
static inline modRM_encoded modRM_value(
    const modRM_decoded modRM,
    const bool use_sib
) {
    modRM_encoded result = {
        .reg = (modRM.has_digit) ? modRM.digit & 0b111 : modRM.reg.num & 0b111,
        .rm  = use_sib ? 0b100 : modRM.rm.num & 0b111
    };

    switch (modRM.mode) {
        case ADDR_MODE_INDIRECT: {
            result.mod = 0b00;
            break;
        }
        case ADDR_MODE_INDIRECT_DISP8: {
            result.mod = 0b01;
            break;
        }
        case ADDR_MODE_INDIRECT_DISP32: {
            result.mod = 0b10;
            break;
        }
        case ADDR_MODE_DIRECT: {
            result.mod = 0b11;
            result.rm  = modRM.rm.num & 0b111;
            break;
        }
        case ADDR_MODE_DISP32_ONLY: {
            result.mod = 0b00;
            result.rm  = 0b101;
            break;
        }
    }

    return result;
}

// emit_encode drives the common byte-writing sequence:
//   [66h] [REX] opcode(s) [ModRM] [displacement bytes] [immediate bytes] INSTRUCTION_END
static emit_result_t emit_encode(const emit_descriptor_t *descriptor) {
    emit_result_t result = {
        .error = EMIT_OK
    };
    uint8_t *instruction = result.instruction_bytes;

    // 66h prefix
    if (descriptor->emit_66h)
        *instruction++ = 0x66;

    // REX — when a ModRM byte is present the same registers drive both REX and
    // ModRM, so read them from there; otherwise use the dedicated rex_* fields
    // (for instructions that embed a register in the opcode, e.g. B0+rb).
    const reg_info_t rex_reg_src = descriptor->has_modRM ? descriptor->modRM.reg : descriptor->rex_reg;
    const reg_info_t rex_rm_src  = descriptor->has_modRM ? descriptor->modRM.rm  : descriptor->rex_rm;
    const uint8_t rex = rex_value(
        rex_reg_src,
        rex_rm_src,
        descriptor->sib.index,
        descriptor->sib.base
    );
    if (rex) {
        *instruction++ = rex;
    }

    // Opcode
    for (uint8_t i = 0; i < descriptor->num_opcodes; i++) {
        *instruction++ = descriptor->opcodes[i];
    }

    // ModRM
    if (descriptor->has_modRM) {
        const modRM_encoded mr = modRM_value(
            descriptor->modRM,
            descriptor->has_sib
        );
        *instruction++ = (mr.mod << 6) | (mr.reg << 3) | mr.rm;
    }

    // SIB
    if (descriptor->has_sib) {
        const uint8_t scale = (descriptor->sib.scale == 1) ? 0 :
                        (descriptor->sib.scale == 2) ? 1 :
                        (descriptor->sib.scale == 4) ? 2 : 3;
        const uint8_t sib = (
            (scale << 6) |
            (descriptor->sib.index.num & 0b111) << 3 |
            (descriptor->sib.base.num & 0b111)
        );
        *instruction++ = sib;
    }

    // disp8/disp32
    if (descriptor->has_modRM) {
        if (descriptor->modRM.mode == ADDR_MODE_INDIRECT_DISP8) {
            *instruction++ = descriptor->disp8;
        }
        else if (
            descriptor->modRM.mode == ADDR_MODE_INDIRECT_DISP32 ||
            descriptor->modRM.mode == ADDR_MODE_DISP32_ONLY
        ) {
            *(uint32_t*)instruction = descriptor->disp32;
            instruction += 4;
        }
    }

    // imm8/imm32
    for (uint8_t i = 0; i < descriptor->imm_len; i++)
        *instruction++ = descriptor->imm_bytes[i];

    *instruction++ = INSTRUCTION_END;
    (void)instruction;

    return result;
}

#endif //MOS6502_HELPERS_H