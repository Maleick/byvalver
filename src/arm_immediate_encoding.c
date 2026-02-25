/*
 * ARM Immediate Encoding Helpers Implementation
 */

#include "arm_immediate_encoding.h"
#include <stdlib.h>
#include "utils.h"  // For is_bad_byte_free

/**
 * Check if a 32-bit value can be encoded as an ARM immediate.
 * ARM immediates: 8-bit value rotated right by 0, 2, 4, ..., 30 bits.
 */
static uint32_t ror32(uint32_t value, unsigned int shift) {
    shift &= 31U;
    if (shift == 0U) {
        return value;
    }
    return (value >> shift) | (value << (32U - shift));
}

int is_arm_immediate_encodable(uint32_t value) {
    // Try all possible rotations (0, 2, 4, ..., 30)
    for (int rotation = 0; rotation < 32; rotation += 2) {
        // Rotate right by 'rotation' bits
        uint32_t rotated = ror32(value, (unsigned int)rotation);
        // Check if the high 24 bits are zero (fits in 8 bits)
        if ((rotated & 0xFFFFFF00) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Encode a 32-bit value as ARM immediate encoding.
 * Returns the encoded immediate (8-bit value + 4-bit rotation) or -1 if impossible.
 */
int encode_arm_immediate(uint32_t value) {
    // Try all possible rotations
    for (int rotation = 0; rotation < 32; rotation += 2) {
        // Rotate right by 'rotation' bits
        uint32_t rotated = ror32(value, (unsigned int)rotation);
        // Check if it fits in 8 bits
        if ((rotated & 0xFFFFFF00) == 0) {
            uint8_t imm8 = rotated & 0xFF;
            uint8_t rot = (rotation / 2) & 0xF;  // Rotation in 2-bit units
            return (rot << 8) | imm8;
        }
    }
    return -1;  // Cannot encode
}

/**
 * Find MVN equivalent for values that can't be encoded as MOV immediate.
 * MVN (Move NOT) can represent ~value, which might be encodable when value isn't.
 */
int find_arm_mvn_immediate(uint32_t target, uint32_t *mvn_val_out) {
    uint32_t mvn_target = ~target;  // MVN value
    if (is_arm_immediate_encodable(mvn_target)) {
        *mvn_val_out = mvn_target;
        return 1;
    }
    return 0;
}

/**
 * Find a split target = part1 + part2 where both parts are immediate-encodable
 * and individually bad-byte-safe.
 */
int find_arm_addsub_split_immediate(uint32_t target, uint32_t *part1_out, uint32_t *part2_out) {
    if (!part1_out || !part2_out) {
        return 0;
    }

    // If target itself is already a "safe immediate", no split needed.
    if (is_arm_immediate_encodable(target) && is_bad_byte_free(target)) {
        return 0;
    }

    // Enumerate all ARM-immediate values: ror(imm8, rot*2), imm8 != 0.
    for (uint32_t rot = 0; rot < 16; rot++) {
        for (uint32_t imm8 = 1; imm8 <= 0xFF; imm8++) {
            uint32_t part1 = ror32(imm8, rot * 2);
            if (!is_bad_byte_free(part1) || part1 >= target) {
                continue;
            }

            uint32_t part2 = target - part1;
            if (!is_bad_byte_free(part2) || !is_arm_immediate_encodable(part2)) {
                continue;
            }

            *part1_out = part1;
            *part2_out = part2;
            return 1;
        }
    }

    return 0;
}

int is_arm_displacement_encodable(int32_t displacement) {
    return displacement >= -4095 && displacement <= 4095;
}

/**
 * Find a two-step displacement split displacement = pre_adjust + residual
 * where both parts fit ARM +/- imm12 offset limits and are bad-byte-safe.
 */
int find_arm_displacement_split(int32_t displacement, int32_t *pre_adjust_out, int32_t *residual_out) {
    if (!pre_adjust_out || !residual_out) {
        return 0;
    }
    if (!is_arm_displacement_encodable(displacement) || displacement == 0) {
        return 0;
    }

    for (int32_t pre = -1024; pre <= 1024; pre++) {
        if (pre == 0) {
            continue;
        }

        int32_t residual = displacement - pre;
        if (!is_arm_displacement_encodable(pre) || !is_arm_displacement_encodable(residual)) {
            continue;
        }
        if (!is_bad_byte_free((uint32_t)pre) || !is_bad_byte_free((uint32_t)residual)) {
            continue;
        }

        *pre_adjust_out = pre;
        *residual_out = residual;
        return 1;
    }

    return 0;
}

int plan_arm_displacement_rewrite(int32_t displacement, int32_t *pre_adjust_out, int32_t *residual_out,
                                  uint32_t *pre_magnitude_out, uint8_t *pre_opcode_out) {
    int32_t pre_adjust, residual;
    uint32_t magnitude;
    uint8_t opcode;

    if (!pre_adjust_out || !residual_out || !pre_magnitude_out || !pre_opcode_out) {
        return 0;
    }
    if (!find_arm_displacement_split(displacement, &pre_adjust, &residual)) {
        return 0;
    }

    opcode = (pre_adjust >= 0) ? 0x4U : 0x2U;  // ADD or SUB
    magnitude = (uint32_t)(pre_adjust >= 0 ? pre_adjust : -pre_adjust);
    if (!is_arm_immediate_encodable(magnitude) || !is_bad_byte_free(magnitude)) {
        return 0;
    }

    *pre_adjust_out = pre_adjust;
    *residual_out = residual;
    *pre_magnitude_out = magnitude;
    *pre_opcode_out = opcode;
    return 1;
}

int encode_arm_dp_immediate(uint8_t cond, uint8_t opcode, uint8_t rn, uint8_t rd,
                            uint32_t imm, int set_flags, uint32_t *instruction_out) {
    int encoded_imm;
    uint32_t instruction;

    if (!instruction_out || cond > 0xF) {
        return 0;
    }

    encoded_imm = encode_arm_immediate(imm);
    if (encoded_imm < 0) {
        return 0;
    }

    instruction = ((uint32_t)cond << 28) |
                  (1U << 25) |
                  ((uint32_t)(opcode & 0xF) << 21) |
                  ((uint32_t)(set_flags ? 1 : 0) << 20) |
                  ((uint32_t)(rn & 0xF) << 16) |
                  ((uint32_t)(rd & 0xF) << 12) |
                  (uint32_t)(encoded_imm & 0xFFF);

    *instruction_out = instruction;
    return 1;
}

int encode_arm_ldr_str_immediate(uint8_t cond, int is_load, uint8_t rn, uint8_t rd,
                                 int32_t displacement, uint32_t *instruction_out) {
    uint32_t instruction;
    uint32_t imm12;
    uint32_t up_bit;

    if (!instruction_out || cond > 0xF || !is_arm_displacement_encodable(displacement)) {
        return 0;
    }

    up_bit = (displacement >= 0) ? 1U : 0U;
    imm12 = (uint32_t)(displacement >= 0 ? displacement : -displacement) & 0xFFF;

    instruction = ((uint32_t)cond << 28) |
                  0x04000000U |         // Single data transfer class
                  (1U << 24) |          // P=1 (pre-indexed)
                  (up_bit << 23) |      // U bit
                  ((uint32_t)(is_load ? 1 : 0) << 20) |
                  ((uint32_t)(rn & 0xF) << 16) |
                  ((uint32_t)(rd & 0xF) << 12) |
                  imm12;

    *instruction_out = instruction;
    return 1;
}

int decode_arm_branch_offset(uint32_t instruction, int32_t *word_offset_out) {
    int32_t imm24;
    if (!word_offset_out) {
        return 0;
    }

    imm24 = (int32_t)(instruction & 0x00FFFFFFU);
    if (imm24 & 0x00800000) {
        imm24 |= (int32_t)0xFF000000U;
    }

    if (!is_arm_branch_word_offset_encodable(imm24)) {
        return 0;
    }

    *word_offset_out = imm24;
    return 1;
}

int is_arm_branch_word_offset_encodable(int32_t word_offset) {
    return word_offset >= ARM_BRANCH_WORD_OFFSET_MIN &&
           word_offset <= ARM_BRANCH_WORD_OFFSET_MAX;
}

int encode_arm_branch_instruction(uint8_t cond, int32_t word_offset, uint32_t *instruction_out) {
    if (!instruction_out || cond > 0xF) {
        return 0;
    }
    if (!is_arm_branch_word_offset_encodable(word_offset)) {
        return 0;
    }

    *instruction_out = ((uint32_t)cond << 28) |
                       0x0A000000U |
                       ((uint32_t)word_offset & 0x00FFFFFFU);
    return 1;
}

int plan_arm_branch_conditional_alt_offsets(int32_t original_word_offset,
                                            int32_t *skip_word_offset_out,
                                            int32_t *taken_word_offset_out) {
    int32_t rewritten_word_offset;

    if (!skip_word_offset_out || !taken_word_offset_out) {
        return 0;
    }
    if (!is_arm_branch_word_offset_encodable(original_word_offset)) {
        return 0;
    }
    if (original_word_offset <= ARM_BRANCH_WORD_OFFSET_MIN) {
        return 0;
    }

    rewritten_word_offset = original_word_offset - 1;
    if (!is_arm_branch_word_offset_encodable(rewritten_word_offset)) {
        return 0;
    }

    *skip_word_offset_out = 0;
    *taken_word_offset_out = rewritten_word_offset;
    return 1;
}

int invert_arm_condition(uint8_t cond, uint8_t *inverted_out) {
    if (!inverted_out) {
        return 0;
    }

    switch (cond & 0xF) {
        case 0x0: *inverted_out = 0x1; return 1;  // EQ <-> NE
        case 0x1: *inverted_out = 0x0; return 1;
        case 0x2: *inverted_out = 0x3; return 1;  // CS/HS <-> CC/LO
        case 0x3: *inverted_out = 0x2; return 1;
        case 0x4: *inverted_out = 0x5; return 1;  // MI <-> PL
        case 0x5: *inverted_out = 0x4; return 1;
        case 0x6: *inverted_out = 0x7; return 1;  // VS <-> VC
        case 0x7: *inverted_out = 0x6; return 1;
        case 0x8: *inverted_out = 0x9; return 1;  // HI <-> LS
        case 0x9: *inverted_out = 0x8; return 1;
        case 0xA: *inverted_out = 0xB; return 1;  // GE <-> LT
        case 0xB: *inverted_out = 0xA; return 1;
        case 0xC: *inverted_out = 0xD; return 1;  // GT <-> LE
        case 0xD: *inverted_out = 0xC; return 1;
        default: return 0;  // AL/NV not handled
    }
}

/**
 * Map ARM register to index (0-15 for R0-R15)
 */
uint8_t get_arm_reg_index(arm_reg reg) {
    switch (reg) {
        case ARM_REG_R0: return 0;
        case ARM_REG_R1: return 1;
        case ARM_REG_R2: return 2;
        case ARM_REG_R3: return 3;
        case ARM_REG_R4: return 4;
        case ARM_REG_R5: return 5;
        case ARM_REG_R6: return 6;
        case ARM_REG_R7: return 7;
        case ARM_REG_R8: return 8;
        case ARM_REG_R9: return 9;
        case ARM_REG_R10: return 10;
        case ARM_REG_R11: return 11;
        case ARM_REG_R12: return 12;
        case ARM_REG_R13: return 13;
        case ARM_REG_R14: return 14;
        case ARM_REG_R15: return 15;
        default: return 0;  // Default to R0
    }
}

/**
 * Check if ARM instruction has bad bytes in the given profile.
 * This is a simplified check - in practice, we'd need to encode the instruction.
 */
int arm_has_bad_bytes(cs_insn *insn, const bad_byte_config_t *profile) {
    if (!profile || profile->bad_byte_count == 0) {
        return 0;  // No bad bytes to check
    }

    // Check each byte of the instruction encoding
    for (size_t i = 0; i < insn->size; i++) {
        uint8_t byte = insn->bytes[i];
        if (profile->bad_bytes[byte]) {
            return 1;  // Found bad byte
        }
    }
    return 0;
}
