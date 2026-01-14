/*
 * ARM Immediate Encoding Helpers Implementation
 */

#include "arm_immediate_encoding.h"
#include <stdlib.h>

/**
 * Check if a 32-bit value can be encoded as an ARM immediate.
 * ARM immediates: 8-bit value rotated right by 0, 2, 4, ..., 30 bits.
 */
int is_arm_immediate_encodable(uint32_t value) {
    // Try all possible rotations (0, 2, 4, ..., 30)
    for (int rotation = 0; rotation < 32; rotation += 2) {
        // Rotate right by 'rotation' bits
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
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
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
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