//
// Created by chinmay on 21/04/26.
//

#include "helpers.h"

// static functions to reduce code duplication
static emit_result_t emit_group_reg(
    const registers_t destination,
    const uint8_t digit,
    const uint8_t group
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = { (destination_info.size == REG_SIZE_BYTE) ? group : group+1 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_group_imm(
    const registers_t destination,
    const uint8_t immediate,
    const uint8_t digit,
    const uint8_t group
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = { (destination_info.size == REG_SIZE_BYTE) ? group : group+1 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .imm_bytes = {immediate},
        .imm_len = 1,
        .has_modRM = true,
        .has_sib = false
    });
}

static emit_result_t emit_group_mem(
    const registers_t destination,
    const mem_ptr_t ptr_size,
    const uint8_t digit,
    const uint8_t group
) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_QWORD) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_ADDRESSING };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (ptr_size == WORD_PTR),
        .opcodes = { (ptr_size == BYTE_PTR) ? group : group+1 },
        .num_opcodes = 1,
        .modRM = {
            .digit = digit,
            .rm = destination_info,
            .mode = ADDR_MODE_INDIRECT,
            .has_digit = true
        },
        .has_modRM = true,
        .has_sib = false
    });
}

// public facing functions
emit_result_t emit_inc_reg(const registers_t destination) {
    return emit_group_reg(destination, 0b000, 0xFE);
}

emit_result_t emit_inc_mem(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b000, 0xFE);
}

emit_result_t emit_dec_reg(const registers_t destination) {
    return emit_group_reg(destination, 0b001, 0xFE);
}

emit_result_t emit_dec_mem(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b001, 0xFE);
}

emit_result_t emit_not_reg(const registers_t destination) {
    return emit_group_reg(destination, 0b010, 0xF6);
}

emit_result_t emit_shl_reg_1(const registers_t destination) {
    return emit_group_reg(destination, 0b100, 0xD0);
}

emit_result_t emit_shl_mem_1(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b100, 0xD0);
}

emit_result_t emit_shl_reg_imm(const registers_t destination, const uint8_t immediate) {
    return emit_group_imm(destination, immediate, 0b100, 0xC0);
}

emit_result_t emit_shr_reg_1(const registers_t destination) {
    return emit_group_reg(destination, 0b101, 0xD0);
}

emit_result_t emit_shr_reg_imm(const registers_t destination, const uint8_t immediate) {
    return emit_group_imm(destination, immediate, 0b101, 0xC0);
}

emit_result_t emit_shr_mem_1(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b101, 0xD0);
}

emit_result_t emit_rcl_reg_1(const registers_t destination) {
    return emit_group_reg(destination, 0b010, 0xD0);
}

emit_result_t emit_rcl_mem_1(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b010, 0xD0);
}

emit_result_t emit_rcr_reg_1(const registers_t destination) {
    return emit_group_reg(destination, 0b011, 0xD0);
}

emit_result_t emit_rcr_mem_1(const registers_t destination, const mem_ptr_t ptr_size) {
    return emit_group_mem(destination, ptr_size, 0b011, 0xD0);
}

emit_result_t emit_test_reg_reg(const registers_t destination, const registers_t source) {
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

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = (destination_info.size == REG_SIZE_WORD),
        .opcodes = {
            (destination_info.size == REG_SIZE_BYTE) ? 0x84 : 0x85
        },
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

emit_result_t emit_test_reg_imm8(const registers_t destination, const uint8_t immediate) {
    const reg_info_t destination_info = reg_info(destination);

    if (
        destination_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_BYTE) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = false,
        .opcodes = {0xF6},
        .num_opcodes = 1,
        .modRM = {
            .digit = 0b000,
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

emit_result_t emit_test_reg_imm16(const registers_t destination, const uint16_t immediate) {
    const reg_info_t destination_info = reg_info(destination);

    if (
        destination_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_WORD) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = true,
        .opcodes = {0xF7},
        .num_opcodes = 1,
        .modRM = {
            .digit = 0b000,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .imm_bytes = {
            immediate & 0xFF,
            (immediate >> 8) & 0xFF,
        },
        .imm_len = 2,
        .has_modRM = true,
        .has_sib = false
    });
}


emit_result_t emit_test_reg_imm32(const registers_t destination, const uint32_t immediate) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_DWORD && destination_info.size != REG_SIZE_QWORD) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = false,
        .opcodes = {0xF7},
        .num_opcodes = 1,
        .modRM = {
            .digit = 0b000,
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