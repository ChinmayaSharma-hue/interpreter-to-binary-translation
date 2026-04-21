//
// Created by chinmay on 21/04/26.
//

#include "helpers.h"

// static functions to reduce code duplication
static emit_result_t emit_alu_reg_reg(
    const registers_t destination,
    const registers_t source,
    const uint8_t opcode_base
) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info = reg_info(source);

    if (
        destination_info.reg_class == REG_CLASS_INVALID ||
        source_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != source_info.size) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };
    }

    const uint8_t width = (destination_info.size != REG_SIZE_BYTE) ? 1 : 0;

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = { opcode_base | (1 << 1) | width },
        .num_opcodes = 1,
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode = ADDR_MODE_DIRECT
        },
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_alu_reg_mem(
    const registers_t destination,
    const registers_t source,
    const uint8_t opcode_base
) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info = reg_info(source);

    if (
        destination_info.reg_class == REG_CLASS_INVALID ||
        source_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (source_info.size != REG_SIZE_QWORD) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_ADDRESSING
        };
    }

    const uint8_t width = (destination_info.size != REG_SIZE_BYTE) ? 1 : 0;

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = {
            opcode_base | (1 << 1) | width
        },
        .num_opcodes = 1,
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode = ADDR_MODE_INDIRECT
        },
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_alu_reg_imm8(
    const registers_t destination,
    const uint8_t immediate,
    const uint8_t digit
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = { (destination_info.size == REG_SIZE_BYTE) ? 0x80 : 0x83 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .imm_bytes = { immediate },
        .imm_len = 1,
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_alu_reg_imm16(
    const registers_t destination,
    const uint16_t immediate,
    const uint8_t digit
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_WORD) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_INVALID };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = true,
        .opcodes = { 0x81 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .imm_bytes = { immediate & 0xFF, (immediate >> 8) & 0xFF },
        .imm_len = 2,
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_alu_reg_imm32(
    const registers_t destination,
    const uint32_t immediate,
    const uint8_t digit
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (
        destination_info.size != REG_SIZE_DWORD &&
        destination_info.size != REG_SIZE_QWORD
    ) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_INVALID };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = false,
        .opcodes = { 0x81 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .imm_bytes = {
            immediate & 0xFF,
            (immediate >> 8) & 0xFF,
            (immediate >> 16) & 0xFF,
            (immediate >> 24) & 0xFF
        },
        .imm_len = 4,
        .has_modRM = true,
        .has_sib = false
    });
}

emit_result_t emit_add_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x00);
}

emit_result_t emit_add_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b000);
}

emit_result_t emit_add_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b000);
}

emit_result_t emit_add_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b000);
}

emit_result_t emit_adc_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x10);
}

emit_result_t emit_adc_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b010);
}

emit_result_t emit_adc_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b010);
}

emit_result_t emit_adc_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b010);
}

emit_result_t emit_adc_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x10);
}

emit_result_t emit_sub_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x28);
}

emit_result_t emit_sub_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b101);
}

emit_result_t emit_sub_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b101);
}

emit_result_t emit_sub_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b101);
}

emit_result_t emit_sub_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x28);
}

emit_result_t emit_cmp_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x38);
}

emit_result_t emit_cmp_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b111);
}

emit_result_t emit_cmp_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b111);
}

emit_result_t emit_cmp_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b111);
}

emit_result_t emit_cmp_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x38);
}

emit_result_t emit_and_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x20);
}

emit_result_t emit_and_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b100);
}

emit_result_t emit_and_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b100);
}

emit_result_t emit_and_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b100);
}

emit_result_t emit_and_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x20);
}

emit_result_t emit_or_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x08);
}

emit_result_t emit_or_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b001);
}

emit_result_t emit_or_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b001);
}

emit_result_t emit_or_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b001);
}

emit_result_t emit_or_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x08);
}

emit_result_t emit_xor_reg_reg(const registers_t destination, const registers_t source) {
    return emit_alu_reg_reg(destination, source, 0x30);
}

emit_result_t emit_xor_reg_imm8(const registers_t destination, const uint8_t immediate) {
    return emit_alu_reg_imm8(destination, immediate, 0b110);
}

emit_result_t emit_xor_reg_imm16(const registers_t destination, const uint16_t immediate) {
    return emit_alu_reg_imm16(destination, immediate, 0b110);
}

emit_result_t emit_xor_reg_imm32(const registers_t destination, const uint32_t immediate) {
    return emit_alu_reg_imm32(destination, immediate, 0b110);
}

emit_result_t emit_xor_reg_mem(const registers_t destination, const registers_t source) {
    return emit_alu_reg_mem(destination, source, 0x30);
}
