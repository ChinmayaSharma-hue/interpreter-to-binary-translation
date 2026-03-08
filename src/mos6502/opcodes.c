//
// Created by chinmay on 01/02/26.
//

#include <stdint.h>
#include "mos6502/opcodes.h"

#include <stdbool.h>
#include "mos6502/cpu.h"

// Addressing helpers
static inline uint16_t addr_abs(chip_t *chip) {
    const uint16_t lo = chip->memory[chip->program_counter++];
    const uint16_t hi = chip->memory[chip->program_counter++];
    const uint16_t address = lo | (hi << 8);
    return address;
}

static inline uint16_t addr_abs_x(chip_t *chip) {
    const uint8_t lo = chip->memory[chip->program_counter++];
    const uint8_t hi = chip->memory[chip->program_counter++];
    const uint16_t address = (lo | (hi << 8)) + chip->index_x_register;
    return address;
}

static inline uint16_t addr_abs_y(chip_t *chip) {
    const uint8_t lo = chip->memory[chip->program_counter++];
    const uint8_t hi = chip->memory[chip->program_counter++];
    const uint16_t address = (lo | (hi << 8)) + chip->index_y_register;
    return address;
}

static inline uint16_t addr_zp(chip_t *chip) {
    const uint16_t address = chip->memory[chip->program_counter++];
    return address;
}

static inline uint16_t addr_zp_x(chip_t *chip) {
    const uint8_t zp = chip->memory[chip->program_counter++];
    const uint16_t address = (zp + chip->index_x_register) & 0xFF;
    return address;
}

static inline uint16_t addr_zp_y(chip_t *chip) {
    const uint8_t zp = chip->memory[chip->program_counter++];
    const uint16_t address = (zp + chip->index_y_register) & 0xFF;
    return address;
}

static inline uint16_t addr_ind_x(chip_t *chip) {
    const uint8_t zp = chip->memory[chip->program_counter++];
    const uint8_t ptr = (zp + chip->index_x_register) & 0xFF;
    const uint8_t lo = chip->memory[ptr];
    const uint8_t hi = chip->memory[(ptr+1) & 0xFF];
    const uint16_t address = lo | (hi << 8);
    return address;
}

static inline uint16_t addr_ind_y(chip_t *chip) {
    const uint8_t zp = chip->memory[chip->program_counter++];
    const uint8_t lo = chip->memory[zp];
    const uint8_t hi = chip->memory[(zp+1) & 0xFF];
    const uint16_t address = (lo | (hi << 8)) + chip->index_y_register;
    return address;
}

static inline void set_ZN(chip_t *chip, const uint8_t value) {
    if (value == 0) {
        chip->status_register |= 0x02;
    } else {
        chip->status_register &= ~0x02;
    }

    if (value & 0x80) {
        chip->status_register |= 0x80;
    } else {
        chip->status_register &= ~0x80;
    }

    chip->status_register |= 0x20;
}

static inline void set_carry(chip_t *chip, const bool cond) {
    if (cond) {
        chip->status_register |= 0x01;
    }
    else {
        chip->status_register &= ~0x01;
    }
}

static inline void set_overflow(chip_t *chip, const bool cond) {
    if (cond) {
        chip->status_register |= 0x40;
    } else {
        chip->status_register &= ~0x40;
    }
}

static inline void set_decimal(chip_t *chip, const bool cond) {
    if (cond) {
        chip->status_register |= 0x8;
    } else {
        chip->status_register &= ~0x8;
    }
}

static inline void set_interrupt_disable(chip_t *chip, const bool cond) {
    if (cond) {
        chip->status_register |= 0x4;
    } else {
        chip->status_register &= ~0x4;
    }
}

// LOAD
static void op_lda_imm(chip_t *chip) {
    chip->accumulator = chip->memory[chip->program_counter++];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_abs(chip_t *chip) {
    chip->accumulator = chip->memory[addr_abs(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_abs_x(chip_t *chip) {
    chip->accumulator = chip->memory[addr_abs_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_abs_y(chip_t *chip) {
    chip->accumulator = chip->memory[addr_abs_y(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_zp(chip_t *chip) {
    chip->accumulator = chip->memory[addr_zp(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_zp_x(chip_t *chip) {
    chip->accumulator = chip->memory[addr_zp_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_ind_x(chip_t *chip) {
    chip->accumulator = chip->memory[addr_ind_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_lda_ind_y(chip_t *chip) {
    chip->accumulator = chip->memory[addr_ind_y(chip)];
    set_ZN(chip, chip->accumulator);
}

static void op_ldx_imm(chip_t *chip) {
    chip->index_x_register = chip->memory[chip->program_counter++];
    set_ZN(chip, chip->index_x_register);
}
static void op_ldx_abs(chip_t *chip) {
    chip->index_x_register = chip->memory[addr_abs(chip)];
    set_ZN(chip, chip->index_x_register);
}
static void op_ldx_abs_y(chip_t *chip) {
    chip->index_x_register = chip->memory[addr_abs_y(chip)];
    set_ZN(chip, chip->index_x_register);
}
static void op_ldx_zp(chip_t *chip) {
    chip->index_x_register = chip->memory[addr_zp(chip)];
    set_ZN(chip, chip->index_x_register);
}
static void op_ldx_zp_y(chip_t *chip) {
    chip->index_x_register = chip->memory[addr_zp_y(chip)];
    set_ZN(chip, chip->index_x_register);
}

static void op_ldy_imm(chip_t *chip) {
    chip->index_y_register = chip->memory[chip->program_counter++];
    set_ZN(chip, chip->index_y_register);
}
static void op_ldy_abs(chip_t *chip) {
    chip->index_y_register = chip->memory[addr_abs(chip)];
    set_ZN(chip, chip->index_y_register);
}
static void op_ldy_abs_x(chip_t *chip) {
    chip->index_y_register = chip->memory[addr_abs_x(chip)];
    set_ZN(chip, chip->index_y_register);
}
static void op_ldy_zp(chip_t *chip) {
    chip->index_y_register = chip->memory[addr_zp(chip)];
    set_ZN(chip, chip->index_y_register);
}
static void op_ldy_zp_x(chip_t *chip) {
    chip->index_y_register = chip->memory[addr_zp_x(chip)];
    set_ZN(chip, chip->index_y_register);
}

// STORE
static void op_sta_abs(chip_t *chip) {
    chip->memory[addr_abs(chip)] = chip->accumulator;
}
static void op_sta_abs_x(chip_t *chip) {
    chip->memory[addr_abs_x(chip)] = chip->accumulator;
}
static void op_sta_abs_y(chip_t *chip) {
    chip->memory[addr_abs_y(chip)] = chip->accumulator;
}
static void op_sta_zp(chip_t *chip) {
    chip->memory[addr_zp(chip)] = chip->accumulator;
}
static void op_sta_zp_x(chip_t *chip) {
    chip->memory[addr_zp_x(chip)] = chip->accumulator;
}
static void op_sta_ind_x(chip_t *chip) {
    chip->memory[addr_ind_x(chip)] = chip->accumulator;
}
static void op_sta_ind_y(chip_t *chip) {
    chip->memory[addr_ind_y(chip)] = chip->accumulator;
}

static void op_stx_abs(chip_t *chip) {
    chip->memory[addr_abs(chip)] = chip->index_x_register;
}
static void op_stx_zp(chip_t *chip) {
    chip->memory[addr_zp(chip)] = chip->index_x_register;
}
static void op_stx_zp_y(chip_t *chip) {
    chip->memory[addr_zp_y(chip)] = chip->index_x_register;
}

static void op_sty_abs(chip_t *chip) {
    chip->memory[addr_abs(chip)] = chip->index_y_register;
}
static void op_sty_zp(chip_t *chip) {
    chip->memory[addr_zp(chip)] = chip->index_y_register;
}
static void op_sty_zp_x(chip_t *chip) {
    chip->memory[addr_zp_x(chip)] = chip->index_y_register;
}

// TRANSFER
static void op_tax(chip_t *chip) {
    chip->index_x_register = chip->accumulator;
    set_ZN(chip, chip->index_x_register);
}
static void op_tay(chip_t *chip) {
    chip->index_y_register = chip->accumulator;
    set_ZN(chip, chip->index_y_register);
}
static void op_tsx(chip_t *chip) {
    chip->index_x_register = chip->stack_pointer;
    set_ZN(chip, chip->index_x_register);
}
static void op_txa(chip_t *chip) {
    chip->accumulator = chip->index_x_register;
    set_ZN(chip, chip->accumulator);
}
static void op_tya(chip_t *chip) {
    chip->accumulator = chip->index_y_register;
    set_ZN(chip, chip->accumulator);
}
static void op_txs(chip_t *chip) {
    chip->stack_pointer = chip->index_x_register;
}

// PUSH/PULL STACK
static void op_pha(chip_t *chip) {
    chip->memory[0x0100 | chip->stack_pointer] = chip->accumulator;
    chip->stack_pointer--;
}
static void op_php(chip_t *chip) {
    chip->memory[0x0100 | chip->stack_pointer] = chip->status_register | 0x30;
    chip->stack_pointer--;
}
static void op_pla(chip_t *chip) {
    chip->stack_pointer++;
    chip->accumulator = chip->memory[0x0100 | chip->stack_pointer];
    set_ZN(chip, chip->accumulator);
}
static void op_plp(chip_t *chip) {
    chip->stack_pointer++;
    chip->status_register = chip->memory[0x0100 | chip->stack_pointer];
    chip->status_register |= 0x30;
}

// ARITHMETIC SHIFT LEFT
static inline void asl_memory_value(chip_t *chip, const uint16_t address) {
    set_carry(chip, chip->memory[address] & 0x80);
    chip->memory[address] = chip->memory[address] << 1;
    set_ZN(chip, chip->memory[address]);
}
static void op_asl_acc(chip_t *chip) {
    set_carry(chip, chip->accumulator & 0x80);
    chip->accumulator = chip->accumulator << 1;
    set_ZN(chip, chip->accumulator);
}
static void op_asl_abs(chip_t *chip) {
    asl_memory_value(chip, addr_abs(chip));
};
static void op_asl_abs_x(chip_t *chip) {
    asl_memory_value(chip, addr_abs_x(chip));
};
static void op_asl_zp(chip_t *chip) {
    asl_memory_value(chip, addr_zp(chip));
};
static void op_asl_zp_x(chip_t *chip) {
    asl_memory_value(chip, addr_zp_x(chip));
};

// LOGICAL SHIFT RIGHT
static inline void lsr_memory_value(chip_t *chip, const uint16_t address) {
    set_carry(chip, chip->memory[address] & 0x1);
    chip->memory[address] = chip->memory[address] >> 1;
    set_ZN(chip, chip->memory[address]);
    chip->status_register &= ~0x80;
}
static void op_lsr_acc(chip_t *chip) {
    set_carry(chip, chip->accumulator & 0x1);
    chip->accumulator = chip->accumulator >> 1;
    set_ZN(chip, chip->accumulator);
    chip->status_register &= ~0x80;
}
static void op_lsr_abs(chip_t *chip) {
    lsr_memory_value(chip, addr_abs(chip));
};
static void op_lsr_abs_x(chip_t *chip) {
    lsr_memory_value(chip, addr_abs_x(chip));
};
static void op_lsr_zp(chip_t *chip) {
    lsr_memory_value(chip, addr_zp(chip));
};
static void op_lsr_zp_x(chip_t *chip) {
    lsr_memory_value(chip, addr_zp_x(chip));
};

// ROTATE LEFT
static inline void rol_memory_value(chip_t *chip, const uint16_t address) {
    const uint8_t carry = chip->status_register & 0x1;
    set_carry(chip, chip->memory[address] & 0x80);
    chip->memory[address] = chip->memory[address] << 1 | carry;
    set_ZN(chip, chip->memory[address]);
}
static void op_rol_acc(chip_t *chip) {
    const uint8_t carry = chip->status_register & 0x1;
    set_carry(chip, chip->accumulator & 0x80);
    chip->accumulator = chip->accumulator << 1 | carry;
    set_ZN(chip, chip->accumulator);
}
static void op_rol_abs(chip_t *chip) {
    rol_memory_value(chip, addr_abs(chip));
};
static void op_rol_abs_x(chip_t *chip) {
    rol_memory_value(chip, addr_abs_x(chip));
};
static void op_rol_zp(chip_t *chip) {
    rol_memory_value(chip, addr_zp(chip));
};
static void op_rol_zp_x(chip_t *chip) {
    rol_memory_value(chip, addr_zp_x(chip));
};

// ROTATE RIGHT
static inline void ror_memory_value(chip_t *chip, const uint16_t address) {
    const uint8_t carry_in  = (chip->status_register & 0x01) << 7;
    set_carry(chip, chip->memory[address] & 0x1);
    chip->memory[address] = chip->memory[address] >> 1 | carry_in;
    set_ZN(chip, chip->memory[address]);
}
static void op_ror_acc(chip_t *chip) {
    const uint8_t carry_in  = (chip->status_register & 0x01) << 7;
    set_carry(chip, chip->accumulator & 0x1);
    chip->accumulator = chip->accumulator >> 1 | carry_in;
    set_ZN(chip, chip->accumulator);
}
static void op_ror_abs(chip_t *chip) {
    ror_memory_value(chip, addr_abs(chip));
};
static void op_ror_abs_x(chip_t *chip) {
    ror_memory_value(chip, addr_abs_x(chip));
};
static void op_ror_zp(chip_t *chip) {
    ror_memory_value(chip, addr_zp(chip));
};
static void op_ror_zp_x(chip_t *chip) {
    ror_memory_value(chip, addr_zp_x(chip));
};

// AND
static void op_and_imm(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[chip->program_counter++];
    set_ZN(chip, chip->accumulator);
}
static void op_and_abs(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_abs(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_abs_x(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_abs_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_abs_y(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_abs_y(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_zp(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_zp(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_zp_x(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_zp_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_ind_x(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_ind_x(chip)];
    set_ZN(chip, chip->accumulator);
}
static void op_and_ind_y(chip_t *chip) {
    chip->accumulator = chip->accumulator & chip->memory[addr_ind_y(chip)];
    set_ZN(chip, chip->accumulator);
}

// TEST
static inline void bit_memory_value(chip_t* chip, const uint16_t address) {
    if ((chip->accumulator & chip->memory[address]) == 0)
        chip->status_register |= 0x02;
    else
        chip->status_register &= ~0x02;

    if (chip->memory[address] & 0x80)
        chip->status_register |= 0x80;
    else
        chip->status_register &= ~0x80;

    if (chip->memory[address] & 0x40)
        chip->status_register |= 0x40;
    else
        chip->status_register &= ~0x40;

    chip->status_register |= 0x20;
}
static void op_bit_abs(chip_t *chip) {
    bit_memory_value(chip, addr_abs(chip));
}
static void op_bit_zp(chip_t *chip) {
    bit_memory_value(chip, addr_zp(chip));
}

// EXCLUSIVE OR
static inline void eor_memory_value(chip_t* chip, const uint16_t address) {
    chip->accumulator = chip->accumulator ^ chip->memory[address];
    set_ZN(chip, chip->accumulator);
}
static void op_eor_imm(chip_t *chip) {
    chip->accumulator = chip->accumulator ^ chip->memory[chip->program_counter++];
    set_ZN(chip, chip->accumulator);
}
static void op_eor_abs(chip_t *chip) {
    eor_memory_value(chip, addr_abs(chip));
}
static void op_eor_abs_x(chip_t *chip) {
    eor_memory_value(chip, addr_abs_x(chip));
}
static void op_eor_abs_y(chip_t *chip) {
    eor_memory_value(chip, addr_abs_y(chip));
}
static void op_eor_zp(chip_t *chip) {
    eor_memory_value(chip, addr_zp(chip));
}
static void op_eor_zp_x(chip_t *chip) {
    eor_memory_value(chip, addr_zp_x(chip));
}
static void op_eor_ind_x(chip_t *chip) {
    eor_memory_value(chip, addr_ind_x(chip));
}
static void op_eor_ind_y(chip_t *chip) {
    eor_memory_value(chip, addr_ind_y(chip));
}

// OR
static inline void or_memory_value(chip_t* chip, const uint16_t address) {
    chip->accumulator = chip->accumulator | chip->memory[address];
    set_ZN(chip, chip->accumulator);
}
static void op_ora_imm(chip_t *chip) {
    chip->accumulator = chip->accumulator | chip->memory[chip->program_counter++];
    set_ZN(chip, chip->accumulator);
}
static void op_ora_abs(chip_t *chip) {
    or_memory_value(chip, addr_abs(chip));
}
static void op_ora_abs_x(chip_t *chip) {
    or_memory_value(chip, addr_abs_x(chip));
}
static void op_ora_abs_y(chip_t *chip) {
    or_memory_value(chip, addr_abs_y(chip));
}
static void op_ora_zp(chip_t *chip) {
    or_memory_value(chip, addr_zp(chip));
}
static void op_ora_zp_x(chip_t *chip) {
    or_memory_value(chip, addr_zp_x(chip));
}
static void op_ora_ind_x(chip_t *chip) {
    or_memory_value(chip, addr_ind_x(chip));
}
static void op_ora_ind_y(chip_t *chip) {
    or_memory_value(chip, addr_ind_y(chip));
}

// ADD WITH CARRY
static inline void adc(chip_t* chip, const uint8_t value) {
    const uint8_t accumulator = chip->accumulator;
    const uint8_t carry = chip->status_register & 0x01;
    uint16_t sum = accumulator + value + carry;
    const bool overflow = ~(accumulator ^ value) & (accumulator ^ (uint8_t)sum) & 0x80;
    if (chip->status_register & 0x08) {
        // decimal mode
        if (((accumulator & 0x0F) + (value & 0x0F) + carry) > 9) {
            sum += 0x06;
        }
        if (sum > 0x99) {
            sum += 0x60;
        }
        chip->accumulator = sum & 0xff;
        set_carry(chip, sum > 0x99);
    } else {
        // binary mode
        chip->accumulator = sum & 0xFF;
        set_carry(chip, sum & 0x100);
    }
    set_ZN(chip, chip->accumulator);
    set_overflow(chip, overflow);
}
static void op_adc_imm(chip_t *chip) {
    adc(chip, chip->memory[chip->program_counter++]);
}
static void op_adc_abs(chip_t *chip) {
    adc(chip, chip->memory[addr_abs(chip)]);
}
static void op_adc_abs_x(chip_t *chip) {
    adc(chip, chip->memory[addr_abs_x(chip)]);
}
static void op_adc_abs_y(chip_t *chip) {
    adc(chip, chip->memory[addr_abs_y(chip)]);
}
static void op_adc_zp(chip_t *chip) {
    adc(chip, chip->memory[addr_zp(chip)]);
}
static void op_adc_zp_x(chip_t *chip) {
    adc(chip, chip->memory[addr_zp_x(chip)]);
}
static void op_adc_ind_x(chip_t *chip) {
    adc(chip, chip->memory[addr_ind_x(chip)]);
}
static void op_adc_ind_y(chip_t *chip) {
    adc(chip, chip->memory[addr_ind_y(chip)]);
}

// COMPARE
static inline void cmp_acc(chip_t* chip, const uint8_t value) {
    const uint16_t difference = (uint16_t)chip->accumulator - (uint16_t)value;
    set_ZN(chip, (uint8_t)difference);
    set_carry(chip, chip->accumulator >= value);
}
static void op_cmp_imm(chip_t *chip) {
    cmp_acc(chip, chip->memory[chip->program_counter++]);
}
static void op_cmp_abs(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_abs(chip)]);
}
static void op_cmp_abs_x(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_abs_x(chip)]);
}
static void op_cmp_abs_y(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_abs_y(chip)]);
}
static void op_cmp_zp(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_zp(chip)]);
}
static void op_cmp_zp_x(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_zp_x(chip)]);
}
static void op_cmp_ind_x(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_ind_x(chip)]);
}
static void op_cmp_ind_y(chip_t *chip) {
    cmp_acc(chip, chip->memory[addr_ind_y(chip)]);
}

static inline void cmp_x(chip_t* chip, const uint8_t value) {
    const uint8_t difference = chip->index_x_register - value;
    set_ZN(chip, difference);
    set_carry(chip, chip->index_x_register >= value);
}
static void op_cpx_imm(chip_t *chip) {
    cmp_x(chip, chip->memory[chip->program_counter++]);
}
static void op_cpx_abs(chip_t *chip) {
    cmp_x(chip, chip->memory[addr_abs(chip)]);
}
static void op_cpx_zp(chip_t *chip) {
    cmp_x(chip, chip->memory[addr_zp(chip)]);
}

static inline void cmp_y(chip_t* chip, const uint8_t value) {
    const uint8_t difference = chip->index_y_register - value;
    set_ZN(chip, difference);
    set_carry(chip, chip->index_y_register >= value);
}
static void op_cpy_imm(chip_t *chip) {
    cmp_y(chip, chip->memory[chip->program_counter++]);
}
static void op_cpy_abs(chip_t *chip) {
    cmp_y(chip, chip->memory[addr_abs(chip)]);
}
static void op_cpy_zp(chip_t *chip) {
    cmp_y(chip, chip->memory[addr_zp(chip)]);
}

// SUBTRACT WITH CARRY
static inline void sbc(chip_t* chip, const uint8_t value) {
    const uint8_t accumulator = chip->accumulator;
    const uint8_t carry = chip->status_register & 0x01;
    uint16_t difference = accumulator + ~value + carry;
    const bool overflow = ((accumulator ^ difference) & (accumulator ^ value)) & 0x80;
    if (chip->status_register & 0x08) {
        // decimal mode
        if ((accumulator & 0x0F) < ((value & 0x0F) + (1 - carry)))
            difference -= 0x06;
        if (difference > 0xFF)
            difference -= 0x60;
    }
    chip->accumulator = difference & 0xFF;
    set_carry(chip, difference < 0x100);
    set_ZN(chip, chip->accumulator);
    set_overflow(chip, overflow);

}
static void op_sbc_imm(chip_t *chip) {
    sbc(chip, chip->memory[chip->program_counter++]);
}
static void op_sbc_abs(chip_t *chip) {
    sbc(chip, chip->memory[addr_abs(chip)]);
}
static void op_sbc_abs_x(chip_t *chip) {
    sbc(chip, chip->memory[addr_abs_x(chip)]);
}
static void op_sbc_abs_y(chip_t *chip) {
    sbc(chip, chip->memory[addr_abs_y(chip)]);
}
static void op_sbc_zp(chip_t *chip) {
    sbc(chip, chip->memory[addr_zp(chip)]);
}
static void op_sbc_zp_x(chip_t *chip) {
    sbc(chip, chip->memory[addr_zp_x(chip)]);
}
static void op_sbc_ind_x(chip_t *chip) {
    sbc(chip, chip->memory[addr_ind_x(chip)]);
}
static void op_sbc_ind_y(chip_t *chip) {
    sbc(chip, chip->memory[addr_ind_y(chip)]);
}

// DECREMENT
static inline void dec(chip_t* chip, const uint16_t address) {
    const uint16_t value = --chip->memory[address];
    set_ZN(chip, value);
}
static void op_dec_abs(chip_t *chip) {
    dec(chip, addr_abs(chip));
}
static void op_dec_abs_x(chip_t *chip) {
    dec(chip, addr_abs_x(chip));
}
static void op_dec_zp(chip_t *chip) {
    dec(chip, addr_zp(chip));
}
static void op_dec_zp_x(chip_t *chip) {
    dec(chip, addr_zp_x(chip));
}

static void op_dex(chip_t *chip) {
    chip->index_x_register--;
    set_ZN(chip, chip->index_x_register);
}
static void op_dey(chip_t *chip) {
    chip->index_y_register--;
    set_ZN(chip, chip->index_y_register);
}

// INCREMENT
static inline void inc(chip_t* chip, const uint16_t address) {
    const uint16_t value = ++chip->memory[address];
    set_ZN(chip, value);
}
static void op_inc_abs(chip_t *chip) {
    inc(chip, addr_abs(chip));
}
static void op_inc_abs_x(chip_t *chip) {
    inc(chip, addr_abs_x(chip));
}
static void op_inc_zp(chip_t *chip) {
    inc(chip, addr_zp(chip));
}
static void op_inc_zp_x(chip_t *chip) {
    inc(chip, addr_zp_x(chip));
}

static void op_inx(chip_t *chip) {
    chip->index_x_register++;
    set_ZN(chip, chip->index_x_register);
}
static void op_iny(chip_t *chip) {
    chip->index_y_register++;
    set_ZN(chip, chip->index_y_register);
}

// BREAK
static void op_brk(chip_t *chip) {
    const uint16_t return_addr = chip->program_counter+1;
    chip->memory[0x0100 | chip->stack_pointer] = (return_addr >> 8);
    chip->stack_pointer--;
    chip->memory[0x0100 | chip->stack_pointer] = (return_addr & 0xFF);
    chip->stack_pointer--;
    chip->memory[0x0100 | chip->stack_pointer] = chip->status_register | 0x30;
    chip->stack_pointer--;

    chip->status_register |= 0x04;
    const uint8_t lo = chip->memory[0xFFFE];
    const uint8_t hi = chip->memory[0xFFFF];
    chip->program_counter = lo | (hi << 8);
}

// JUMP
static void op_jmp_abs(chip_t *chip) {
    chip->program_counter = addr_abs(chip);
}
static void op_jmp_ind(chip_t *chip) {
    const uint16_t ptr = addr_abs(chip);

    const uint8_t lo = chip->memory[ptr];
    const uint8_t hi = chip->memory[(ptr & 0xFF00) | ((ptr + 1) & 0x00FF)];
    chip->program_counter = lo | (hi << 8);
}

static void op_jsr_abs(chip_t *chip) {
    const uint16_t target = addr_abs(chip);
    const uint16_t return_addr = chip->program_counter - 1;

    chip->memory[0x0100 | chip->stack_pointer] = (return_addr >> 8);
    chip->stack_pointer--;

    chip->memory[0x0100 | chip->stack_pointer] = (return_addr & 0xFF);
    chip->stack_pointer--;

    chip->program_counter = target;
}

// RETURN
static void op_rti(chip_t *chip) {
    chip->stack_pointer++;
    chip->status_register = chip->memory[0x0100 | chip->stack_pointer];
    chip->status_register |= 0x30;

    chip->stack_pointer++;
    const uint8_t lo = chip->memory[0x0100 | chip->stack_pointer];

    chip->stack_pointer++;
    const uint8_t hi = chip->memory[0x0100 | chip->stack_pointer];

    chip->program_counter = lo | (hi << 8);
}
static void op_rts(chip_t *chip) {
    chip->stack_pointer++;
    const uint8_t lo = chip->memory[0x0100 | chip->stack_pointer];

    chip->stack_pointer++;
    const uint8_t hi = chip->memory[0x0100 | chip->stack_pointer];

    const uint16_t return_addr = lo | (hi << 8);
    chip->program_counter = return_addr+1;
}

// BRANCH
static void op_bcc(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (!(chip->status_register & 0x1)) {
        chip->program_counter += offset;
    }
}
static void op_bcs(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (chip->status_register & 0x1) {
        chip->program_counter += offset;
    }
}
static void op_beq(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (chip->status_register & 0x2) {
        chip->program_counter += offset;
    }
}
static void op_bmi(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (chip->status_register & 0x80) {
        chip->program_counter += offset;
    }
}
static void op_bne(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (!(chip->status_register & 0x2)) {
        chip->program_counter = (uint16_t)(chip->program_counter + offset);
    }
}
static void op_bpl(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (!(chip->status_register & 0x80)) {
        chip->program_counter += offset;
    }
}
static void op_bvc(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (!(chip->status_register & 0x40)) {
        chip->program_counter += offset;
    }
}
static void op_bvs(chip_t *chip) {
    const int8_t offset = (int8_t)chip->memory[chip->program_counter++];
    if (chip->status_register & 0x40) {
        chip->program_counter += offset;
    }
}

// SET/CLEAR
static void op_clc(chip_t *chip) {
    set_carry(chip, false);
}
static void op_cld(chip_t *chip) {
    set_decimal(chip, false);
}
static void op_cli(chip_t *chip) {
    set_interrupt_disable(chip, false);
}
static void op_clv(chip_t *chip) {
    set_overflow(chip, false);
}
static void op_sec(chip_t *chip) {
    set_carry(chip, true);
}
static void op_sed(chip_t *chip) {
    set_decimal(chip, true);
}
static void op_sei(chip_t *chip) {
    set_interrupt_disable(chip, true);
}

// NOP
static void op_nop(chip_t *chip) {
    // do nothing at all
}

// Opcode Table
opcode_func opcode_table[256] = {

    /* BRK / NOP */
    [0x00] = op_brk,
    [0xEA] = op_nop,

    /* LDA */
    [0xA9] = op_lda_imm,
    [0xAD] = op_lda_abs,
    [0xBD] = op_lda_abs_x,
    [0xB9] = op_lda_abs_y,
    [0xA5] = op_lda_zp,
    [0xB5] = op_lda_zp_x,
    [0xA1] = op_lda_ind_x,
    [0xB1] = op_lda_ind_y,

    /* LDX */
    [0xA2] = op_ldx_imm,
    [0xAE] = op_ldx_abs,
    [0xBE] = op_ldx_abs_y,
    [0xA6] = op_ldx_zp,
    [0xB6] = op_ldx_zp_y,

    /* LDY */
    [0xA0] = op_ldy_imm,
    [0xAC] = op_ldy_abs,
    [0xBC] = op_ldy_abs_x,
    [0xA4] = op_ldy_zp,
    [0xB4] = op_ldy_zp_x,

    /* STA */
    [0x8D] = op_sta_abs,
    [0x9D] = op_sta_abs_x,
    [0x99] = op_sta_abs_y,
    [0x85] = op_sta_zp,
    [0x95] = op_sta_zp_x,
    [0x81] = op_sta_ind_x,
    [0x91] = op_sta_ind_y,

    /* STX */
    [0x8E] = op_stx_abs,
    [0x86] = op_stx_zp,
    [0x96] = op_stx_zp_y,

    /* STY */
    [0x8C] = op_sty_abs,
    [0x84] = op_sty_zp,
    [0x94] = op_sty_zp_x,

    /* Transfers */
    [0xAA] = op_tax,
    [0xA8] = op_tay,
    [0xBA] = op_tsx,
    [0x8A] = op_txa,
    [0x98] = op_tya,
    [0x9A] = op_txs,

    /* Stack */
    [0x48] = op_pha,
    [0x08] = op_php,
    [0x68] = op_pla,
    [0x28] = op_plp,

    /* ASL */
    [0x0A] = op_asl_acc,
    [0x0E] = op_asl_abs,
    [0x1E] = op_asl_abs_x,
    [0x06] = op_asl_zp,
    [0x16] = op_asl_zp_x,

    /* LSR */
    [0x4A] = op_lsr_acc,
    [0x4E] = op_lsr_abs,
    [0x5E] = op_lsr_abs_x,
    [0x46] = op_lsr_zp,
    [0x56] = op_lsr_zp_x,

    /* ROL */
    [0x2A] = op_rol_acc,
    [0x2E] = op_rol_abs,
    [0x3E] = op_rol_abs_x,
    [0x26] = op_rol_zp,
    [0x36] = op_rol_zp_x,

    /* ROR */
    [0x6A] = op_ror_acc,
    [0x6E] = op_ror_abs,
    [0x7E] = op_ror_abs_x,
    [0x66] = op_ror_zp,
    [0x76] = op_ror_zp_x,

    /* AND */
    [0x29] = op_and_imm,
    [0x2D] = op_and_abs,
    [0x3D] = op_and_abs_x,
    [0x39] = op_and_abs_y,
    [0x25] = op_and_zp,
    [0x35] = op_and_zp_x,
    [0x21] = op_and_ind_x,
    [0x31] = op_and_ind_y,

    /* BIT */
    [0x2C] = op_bit_abs,
    [0x24] = op_bit_zp,

    /* EOR */
    [0x49] = op_eor_imm,
    [0x4D] = op_eor_abs,
    [0x5D] = op_eor_abs_x,
    [0x59] = op_eor_abs_y,
    [0x45] = op_eor_zp,
    [0x55] = op_eor_zp_x,
    [0x41] = op_eor_ind_x,
    [0x51] = op_eor_ind_y,

    /* ORA */
    [0x09] = op_ora_imm,
    [0x0D] = op_ora_abs,
    [0x1D] = op_ora_abs_x,
    [0x19] = op_ora_abs_y,
    [0x05] = op_ora_zp,
    [0x15] = op_ora_zp_x,
    [0x01] = op_ora_ind_x,
    [0x11] = op_ora_ind_y,

    /* ADC */
    [0x69] = op_adc_imm,
    [0x6D] = op_adc_abs,
    [0x7D] = op_adc_abs_x,
    [0x79] = op_adc_abs_y,
    [0x65] = op_adc_zp,
    [0x75] = op_adc_zp_x,
    [0x61] = op_adc_ind_x,
    [0x71] = op_adc_ind_y,

    /* CMP */
    [0xC9] = op_cmp_imm,
    [0xCD] = op_cmp_abs,
    [0xDD] = op_cmp_abs_x,
    [0xD9] = op_cmp_abs_y,
    [0xC5] = op_cmp_zp,
    [0xD5] = op_cmp_zp_x,
    [0xC1] = op_cmp_ind_x,
    [0xD1] = op_cmp_ind_y,

    /* CPX */
    [0xE0] = op_cpx_imm,
    [0xEC] = op_cpx_abs,
    [0xE4] = op_cpx_zp,

    /* CPY */
    [0xC0] = op_cpy_imm,
    [0xCC] = op_cpy_abs,
    [0xC4] = op_cpy_zp,

    /* SBC */
    [0xE9] = op_sbc_imm,
    [0xED] = op_sbc_abs,
    [0xFD] = op_sbc_abs_x,
    [0xF9] = op_sbc_abs_y,
    [0xE5] = op_sbc_zp,
    [0xF5] = op_sbc_zp_x,
    [0xE1] = op_sbc_ind_x,
    [0xF1] = op_sbc_ind_y,

    /* DEC */
    [0xCE] = op_dec_abs,
    [0xDE] = op_dec_abs_x,
    [0xC6] = op_dec_zp,
    [0xD6] = op_dec_zp_x,

    [0xCA] = op_dex,
    [0x88] = op_dey,

    /* INC */
    [0xEE] = op_inc_abs,
    [0xFE] = op_inc_abs_x,
    [0xE6] = op_inc_zp,
    [0xF6] = op_inc_zp_x,

    [0xE8] = op_inx,
    [0xC8] = op_iny,

    /* JMP / JSR */
    [0x4C] = op_jmp_abs,
    [0x6C] = op_jmp_ind,
    [0x20] = op_jsr_abs,

    /* RTI / RTS */
    [0x40] = op_rti,
    [0x60] = op_rts,

    /* Branches */
    [0x90] = op_bcc,
    [0xB0] = op_bcs,
    [0xF0] = op_beq,
    [0x30] = op_bmi,
    [0xD0] = op_bne,
    [0x10] = op_bpl,
    [0x50] = op_bvc,
    [0x70] = op_bvs,

    /* Flags */
    [0x18] = op_clc,
    [0xD8] = op_cld,
    [0x58] = op_cli,
    [0xB8] = op_clv,

    [0x38] = op_sec,
    [0xF8] = op_sed,
    [0x78] = op_sei,
};

void execute_opcode(const uint8_t opcode, chip_t *chip) {
    opcode_table[opcode](chip);
}
