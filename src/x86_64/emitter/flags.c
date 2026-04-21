//
// Created by chinmay on 21/04/26.
//

#include "helpers.h"

// static functions to reduce code duplication
static emit_result_t emit_set_flags(const registers_t destination, const uint8_t condition_opcode) {
    const reg_info_t destination_info = reg_info(destination);

    if (destination_info.reg_class == REG_CLASS_INVALID) {
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    }
    if (destination_info.size != REG_SIZE_BYTE) {
        return (emit_result_t){ .error = EMIT_ERR_SIZE_INVALID };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h = false,
        .opcodes = { 0x0F, condition_opcode },
        .num_opcodes = 2,
        .modRM = {
            .digit = 0b000,
            .rm = destination_info,
            .mode = ADDR_MODE_DIRECT,
            .has_digit = true
        },
        .has_modRM = true,
        .has_sib = false
    });
}

// public facing functions
emit_result_t emit_setc_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x92);
}

emit_result_t emit_setnc_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x93);
}

emit_result_t emit_sete_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x93);
}

emit_result_t emit_setne_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x93);
}

emit_result_t emit_sets_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x98);
}

emit_result_t emit_seto_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x90);
}

emit_result_t emit_seta_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x97);
}

emit_result_t emit_setz_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x94);
}

emit_result_t emit_setnz_reg(const registers_t destination) {
    return emit_set_flags(destination, 0x95);
}

emit_result_t emit_clc(void) {
    return emit_encode(&(emit_descriptor_t){
        .opcodes     = { 0xF8 },
        .num_opcodes = 1,
        .has_modRM   = false,
        .has_sib     = false,
    });
}

emit_result_t emit_stc(void) {
    return emit_encode(&(emit_descriptor_t){
        .opcodes     = { 0xF9 },
        .num_opcodes = 1,
        .has_modRM   = false,
        .has_sib     = false,
    });
}

emit_result_t emit_nop(void) {
    return emit_encode(&(emit_descriptor_t){
        .opcodes     = { 0x90 },
        .num_opcodes = 1,
        .has_modRM   = false,
        .has_sib     = false,
    });
}

emit_result_t emit_bt_reg_imm8(const registers_t source, const uint8_t bit_index) {
    const reg_info_t source_info = reg_info(source);

    if (source_info.reg_class == REG_CLASS_INVALID)
        return (emit_result_t){ .error = EMIT_ERR_INVALID_REGISTER };
    if (source_info.size == REG_SIZE_BYTE)
        return (emit_result_t){ .error = EMIT_ERR_SIZE_MISMATCH };

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (source_info.size == REG_SIZE_WORD),
        .opcodes     = { 0x0F, 0xBA },
        .num_opcodes = 2,
        .modRM = {
            .digit     = 0b100,
            .rm        = source_info,
            .mode      = ADDR_MODE_DIRECT,
            .has_digit = true,
        },
        .imm_bytes = {
            bit_index
        },
        .imm_len   = 1,
        .has_modRM = true,
        .has_sib   = false,
    });
}
