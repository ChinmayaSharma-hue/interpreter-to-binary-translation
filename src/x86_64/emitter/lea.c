//
// Created by chinmay on 21/04/26.
//

#include "helpers.h"

// static functions to reduce code duplication
static emit_result_t emit_lea_sib_disp(
    const registers_t destination,
    const uint8_t scale,
    const registers_t index,
    const registers_t base,
    const addressing_mode mode,
    const uint8_t disp8,
    const uint32_t disp32
) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t index_info = reg_info(index);
    const reg_info_t base_info = reg_info(base);

    if (destination_info.reg_class == REG_CLASS_INVALID ||
        index_info.reg_class == REG_CLASS_INVALID ||
        base_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_REGISTER
        };
    }
    if (index_info.size != REG_SIZE_QWORD || base_info.size != REG_SIZE_QWORD) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_ADDRESSING
        };
    }
    if ((scale != 1) && (scale != 2) && (scale != 4) && (scale != 8)) {
        return (emit_result_t) {
            .error = EMIT_ERR_INVALID_SCALE
        };
    }
    if (destination_info.size == REG_SIZE_BYTE) {
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .opcodes = {0x8D},
        .num_opcodes = 1,
        .modRM = {
            .reg = destination_info,
            .mode = mode
        },
        .sib = {
            .index = index_info,
            .scale = scale,
            .base = base_info,
        },
        .disp8 = disp8,
        .disp32 = disp32,
        .has_modRM = true,
        .has_sib = true
    });
}

static emit_result_t emit_lea_reg_disp(
    const registers_t destination,
    const registers_t source,
    const addressing_mode mode,
    const uint8_t disp8,
    const uint32_t disp32
) {
    const reg_info_t destination_info = reg_info(destination);
    const reg_info_t source_info = reg_info(source);

    if (destination_info.reg_class == REG_CLASS_INVALID ||
        source_info.reg_class == REG_CLASS_INVALID
    ) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_REGISTER
        };
    }
    if (
        destination_info.size == REG_SIZE_BYTE
    ) {
        return (emit_result_t){
            .error = EMIT_ERR_SIZE_INVALID
        };
    }
    if (
        source_info.size == REG_SIZE_BYTE ||
        source_info.size == REG_SIZE_WORD
    ) {
        return (emit_result_t){
            .error = EMIT_ERR_INVALID_ADDRESSING
        };
    }

    return emit_encode(&(emit_descriptor_t){
        .emit_66h    = (destination_info.size == REG_SIZE_WORD),
        .opcodes = {0x8D},
        .num_opcodes = 1,
        .modRM = {
            .reg = destination_info,
            .rm = source_info,
            .mode = mode
        },
        .disp8 = disp8,
        .disp32 = disp32,
        .has_modRM = true,
        .has_sib = false
    });
}

// public facing functions
emit_result_t emit_lea_sib(
    const registers_t destination,
    const uint8_t scale,
    const registers_t index,
    const registers_t base
) {
    const reg_info_t base_info = reg_info(base);
    const bool is_base_rbp = (base_info.num & 7) == 5;
    const addressing_mode mode = is_base_rbp ? ADDR_MODE_INDIRECT_DISP8 : ADDR_MODE_INDIRECT;

    return emit_lea_sib_disp(
        destination,
        scale,
        index,
        base,
        mode,
        0,
        0
    );
}

emit_result_t emit_lea_sib_disp8(
    const registers_t destination,
    const uint8_t scale,
    const registers_t index,
    const registers_t base,
    const uint8_t displacement
) {
    return emit_lea_sib_disp(
        destination,
        scale,
        index,
        base,
        ADDR_MODE_INDIRECT_DISP8,
        displacement,
        0
    );
}

emit_result_t emit_lea_sib_disp32(
    const registers_t destination,
    const uint8_t scale,
    const registers_t index,
    const registers_t base,
    const uint32_t displacement
) {
    return emit_lea_sib_disp(
        destination,
        scale,
        index,
        base,
        ADDR_MODE_INDIRECT_DISP32,
        0,
        displacement
    );
}

emit_result_t emit_lea_reg_disp8(const registers_t destination, const registers_t source, const uint8_t displacement) {
    return emit_lea_reg_disp(destination, source, ADDR_MODE_INDIRECT_DISP8, displacement, 0);
}

emit_result_t emit_lea_reg_disp32(const registers_t destination, const registers_t source, const uint32_t displacement) {
    return emit_lea_reg_disp(destination, source, ADDR_MODE_INDIRECT_DISP32, 0, displacement);
}
