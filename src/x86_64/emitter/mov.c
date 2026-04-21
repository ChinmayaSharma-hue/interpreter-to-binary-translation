//
// Created by chinmay on 21/04/26.
//

#include "helpers.h"

// public facing functions
emit_result_t emit_mov_reg_reg(const registers_t destination, const registers_t source) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info  = reg_info(source);

    if (destination_info.reg_class == REG_CLASS_INVALID || source_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (destination_info.size != source_info.size)
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };

    const uint8_t width = (destination_info.size != REG_SIZE_BYTE) ? 1 : 0;

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode = ADDR_MODE_DIRECT
        },
        .opcodes     = {
            0x8A | width
        },
        .num_opcodes = 1,
        .has_modRM   = true,
        .has_sib = false
    });
}

emit_result_t emit_mov_reg_imm8(const registers_t destination, const uint8_t immediate) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_REGISTER
        };
    if (destination_info.size != REG_SIZE_BYTE)
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };

    return emit_encode(&(emit_descriptor_t){
        .rex_rm      = destination_info,
        .opcodes     = { 0xB0 | (destination_info.num & 0b111) },
        .num_opcodes = 1,
        .imm_bytes   = { immediate },
        .imm_len     = 1,
        .has_modRM = false,
        .has_sib = false
    });
}

emit_result_t emit_mov_reg_imm32(const registers_t destination, const uint32_t immediate) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (destination_info.size != REG_SIZE_DWORD && destination_info.size != REG_SIZE_QWORD)
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };

    return emit_encode(&(emit_descriptor_t){
        .rex_rm      = destination_info,
        .opcodes     = { 0xB8 | (destination_info.num & 0b111) },
        .num_opcodes = 1,
        .imm_bytes   = {
            immediate & 0xFF,
            (immediate >> 8) & 0xFF,
            (immediate >> 16) & 0xFF,
            (immediate >> 24) & 0xFF
        },
        .imm_len     = 4,
        .has_modRM = false,
        .has_sib = false
    });
}

emit_result_t emit_mov_reg_mem(const registers_t destination, const registers_t source) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info  = reg_info(source);

    if (destination_info.reg_class == REG_CLASS_INVALID || source_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (source_info.size != REG_SIZE_QWORD)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_ADDRESSING };

    const uint8_t width = (destination_info.size != REG_SIZE_BYTE) ? 1 : 0;

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode = ADDR_MODE_INDIRECT,
        },
        .opcodes     = {
            0x8A | width
        },
        .num_opcodes = 1,
        .has_modRM   = true,
        .has_sib = false
    });
}

emit_result_t emit_mov_mem_reg(const registers_t destination, const registers_t source) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info  = reg_info(source);

    if (destination_info.reg_class == REG_CLASS_INVALID || source_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (destination_info.size != REG_SIZE_QWORD)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_ADDRESSING };

    const uint8_t width = (source_info.size != REG_SIZE_BYTE) ? 1 : 0;

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (source_info.size == REG_SIZE_WORD),
        .modRM = {
            .reg = source_info,
            .rm = destination_info,
            .mode  = ADDR_MODE_INDIRECT
        },
        .opcodes     = {
            0x88 | width
        },
        .num_opcodes = 1,
        .has_modRM   = true,
        .has_sib = false
    });
}

emit_result_t emit_movzx_reg_reg(const registers_t destination, const registers_t source) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info  = reg_info(source);

    if (destination_info.reg_class == REG_CLASS_INVALID || source_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (source_info.size > destination_info.size || (source_info.size != REG_SIZE_BYTE && source_info.size != REG_SIZE_WORD))
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode  = ADDR_MODE_DIRECT
        },
        .opcodes     = {
            0x0F, (source_info.size == REG_SIZE_BYTE) ? 0xB6 : 0xB7
        },
        .num_opcodes = 2,
        .has_modRM   = true,
        .has_sib = false
    });
}

emit_result_t emit_movzx_reg_mem(const registers_t destination, const registers_t source, const mem_ptr_t pointer) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info  = reg_info(source);

    if (
        destination_info.reg_class == REG_CLASS_INVALID ||
        source_info.reg_class == REG_CLASS_INVALID
    )
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_REGISTER
        };
    if (
        (pointer != BYTE_PTR && pointer != WORD_PTR) ||
        source_info.size != REG_SIZE_QWORD
    ) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_ADDRESSING
        };
    }
    if (
        (pointer == BYTE_PTR && destination_info.size == REG_SIZE_BYTE) ||
        (pointer == WORD_PTR && destination_info.size <= REG_SIZE_WORD)
    )
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode  = ADDR_MODE_INDIRECT
        },
        .opcodes     = {
            0x0F,
            (pointer == BYTE_PTR) ? 0xB6 : 0xB7
        },
        .num_opcodes = 2,
        .has_modRM   = true,
        .has_sib = false
    });
}