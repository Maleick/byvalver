/*
 * BYVALVER - Mixed Arithmetic Base Obfuscation (Priority 73)
 *
 * Transforms immediate value loads into multi-step arithmetic calculations.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"
#include "obfuscation_strategy_registry.h"

// ============================================================================
// Helper: Generate arithmetic expression for immediate value
// ============================================================================

static void generate_arithmetic_expression(struct buffer *b, uint8_t reg_idx, uint32_t value) {
    // Strategy: Break value into arithmetic operations
    // For small values (0-31), use: XOR reg, reg; ADD reg, value
    // For larger values, use various decompositions

    if (value == 0) {
        // XOR reg, reg
        uint8_t bytes[] = {0x31, (uint8_t)(0xC0 | (reg_idx << 3) | reg_idx)};
        buffer_append(b, bytes, 2);
    } else if (value <= 0x1F) {
        // XOR reg, reg; ADD reg, imm8
        uint8_t xor_bytes[] = {0x31, (uint8_t)(0xC0 | (reg_idx << 3) | reg_idx)};
        buffer_append(b, xor_bytes, 2);
        uint8_t add_bytes[] = {0x83, (uint8_t)(0xC0 | reg_idx), (uint8_t)value};
        buffer_append(b, add_bytes, 3);
    } else if ((value & (value - 1)) == 0 && value != 0) {
        // Power of 2: use shift
        // MOV reg, 1; SHL reg, n
        uint8_t mov_bytes[] = {(uint8_t)(0xB8 + reg_idx), 0x01, 0x00, 0x00, 0x00};
        buffer_append(b, mov_bytes, 5);

        // Calculate shift amount
        int shift = 0;
        uint32_t temp = value;
        while (temp > 1) {
            temp >>= 1;
            shift++;
        }

        // SHL reg, imm8
        uint8_t shl_bytes[] = {0xC1, (uint8_t)(0xE0 | reg_idx), (uint8_t)shift};
        buffer_append(b, shl_bytes, 3);
    } else if (value <= 0xFF) {
        // Single byte: decompose into two operations
        // Example: 0x0B = 0x08 + 0x03
        uint8_t base = (value / 2) & 0xFF;
        uint8_t remainder = value - base;

        // XOR reg, reg; MOV reg_low, base; ADD reg_low, remainder
        uint8_t xor_bytes[] = {0x31, (uint8_t)(0xC0 | (reg_idx << 3) | reg_idx)};
        buffer_append(b, xor_bytes, 2);

        // MOV AL/BL/CL/DL, base (using 8-bit register)
        uint8_t mov_byte = 0xB0 + reg_idx;
        buffer_append(b, &mov_byte, 1);
        buffer_append(b, &base, 1);

        // ADD AL/BL/CL/DL, remainder
        uint8_t add_bytes[] = {0x80, (uint8_t)(0xC0 | reg_idx), remainder};
        buffer_append(b, add_bytes, 3);
    } else {
        // Larger values: decompose using multiplication or shifts
        // For simplicity, use: MOV reg, (value/2); SHL reg, 1; ADD reg, (value%2)
        uint32_t half = value / 2;
        uint32_t remainder = value % 2;

        // MOV reg, half
        uint8_t mov_bytes[] = {(uint8_t)(0xB8 + reg_idx),
                               (uint8_t)(half & 0xFF),
                               (uint8_t)((half >> 8) & 0xFF),
                               (uint8_t)((half >> 16) & 0xFF),
                               (uint8_t)((half >> 24) & 0xFF)};
        buffer_append(b, mov_bytes, 5);

        // SHL reg, 1
        uint8_t shl_bytes[] = {0xD1, (uint8_t)(0xE0 | reg_idx)};
        buffer_append(b, shl_bytes, 2);

        if (remainder) {
            // INC reg
            uint8_t inc_byte = 0x40 + reg_idx;
            buffer_append(b, &inc_byte, 1);
        }
    }
}

// ============================================================================
// Strategy: Mixed Arithmetic Base Obfuscation
// ============================================================================

int can_handle_mixed_arithmetic(cs_insn *insn) {
    if (insn->id != X86_INS_MOV) return 0;

    if (insn->detail->x86.op_count == 2) {
        cs_x86_op *dst = &insn->detail->x86.operands[0];
        cs_x86_op *src = &insn->detail->x86.operands[1];

        // Only handle MOV reg, imm (32-bit registers only)
        if (dst->type == X86_OP_REG && src->type == X86_OP_IMM) {
            // Check if it's a 32-bit register (EAX, EBX, ECX, EDX, ESI, EDI)
            if (dst->reg >= X86_REG_EAX && dst->reg <= X86_REG_EDI) {
                // Only transform small to medium immediates (more efficient)
                return (src->imm >= 0 && src->imm <= 0xFFFF);
            }
        }
    }
    return 0;
}

size_t get_mixed_arithmetic_size(cs_insn *insn) {
    uint32_t value = (uint32_t)insn->detail->x86.operands[1].imm;

    if (value == 0) {
        return 2;  // XOR reg, reg
    } else if (value <= 0x1F) {
        return 5;  // XOR + ADD
    } else if ((value & (value - 1)) == 0 && value != 0) {
        return 8;  // MOV + SHL
    } else if (value <= 0xFF) {
        return 8;  // XOR + MOV byte + ADD
    } else {
        return 13;  // MOV + SHL + INC (worst case)
    }
}

void generate_mixed_arithmetic(struct buffer *b, cs_insn *insn) {
    uint8_t dst_reg = insn->detail->x86.operands[0].reg;
    uint32_t value = (uint32_t)insn->detail->x86.operands[1].imm;
    uint8_t reg_idx = get_reg_index(dst_reg);

    generate_arithmetic_expression(b, reg_idx, value);
}

static strategy_t mixed_arithmetic_strategy = {
    .name = "Mixed Arithmetic Base Obfuscation",
    .can_handle = can_handle_mixed_arithmetic,
    .get_size = get_mixed_arithmetic_size,
    .generate = generate_mixed_arithmetic,
    .priority = 73
};

void register_mixed_arithmetic_base_obfuscation() {
    register_obfuscation_strategy(&mixed_arithmetic_strategy);
}
