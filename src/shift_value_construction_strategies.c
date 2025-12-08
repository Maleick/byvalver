/*
 * Shift-Based Value Construction Strategy
 *
 * PROBLEM: Direct MOV reg, imm32 with immediate values containing null bytes
 *
 * SOLUTION: Use bit shift operations combined with arithmetic to construct
 * values that contain null bytes in their direct encoding.
 *
 * FREQUENCY: Common in shellcode for constructing values efficiently
 * PRIORITY: 78 (Medium-High - efficient for certain value patterns)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00200000 (B8 00 00 20 00 - contains nulls)
 *   Strategy: XOR EAX, EAX; MOV AL, 0x20; SHL EAX, 12 (or similar shifts)
 */

#include "shift_value_construction_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV reg, imm32 with null bytes in immediate that 
 * might benefit from shift construction
 */
int can_handle_shift_value_construction(cs_insn *insn) {
    if (insn->id != X86_INS_MOV ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be MOV register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Only handle 32-bit general purpose registers
    if (dst_op->reg < X86_REG_EAX || dst_op->reg > X86_REG_EDI) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;

    // Check if the immediate contains null bytes
    if (is_null_free(imm)) {
        // Already null-free
        return 0;
    }

    // Check if the value might benefit from shift construction
    // e.g., values with many zeros that can be created with shifts
    return 1;
}

/*
 * Size calculation for shift-based value construction
 */
size_t get_size_shift_value_construction(cs_insn *insn) {
    (void)insn;
    // Clear register + set base value + multiple shifts = ~8-12 bytes typically
    return 10;
}

/*
 * Generate shift-based value construction sequence
 */
void generate_shift_value_construction(struct buffer *b, cs_insn *insn) {
    uint32_t target = (uint32_t)insn->detail->x86.operands[1].imm;
    uint8_t reg = insn->detail->x86.operands[0].reg;

    // Try to find a good shift pattern
    // First, check if the value has a pattern suitable for shifting
    
    // Check if the value is a power of 2 or similar (can be created with SHL)
    for (int shift_amount = 1; shift_amount <= 31; shift_amount++) {
        uint32_t shifted_val = 1U << shift_amount;
        if (shifted_val == target) {
            // Perfect match: single bit set at position shift_amount
            uint8_t xor_reg[] = {0x31, 0xC0};  // XOR reg, reg
            xor_reg[1] = (get_reg_index(reg) << 3) | get_reg_index(reg);
            buffer_append(b, xor_reg, 2);
            
            // OR reg, 1  (set bit 0)
            uint8_t or_reg[] = {0x83, 0xC8, 0x01};  // OR reg, 1
            or_reg[1] = 0xC8 + get_reg_index(reg);
            buffer_append(b, or_reg, 3);
            
            // SHL reg, shift_amount
            if (shift_amount <= 255) {
                uint8_t shl_reg[] = {0xC1, 0xE0, 0x00};  // SHL reg, imm8
                shl_reg[1] = 0xE0 + get_reg_index(reg);
                shl_reg[2] = shift_amount;
                buffer_append(b, shl_reg, 3);
            } else {
                // Use CL register for shift > 255
                // MOV CL, shift_amount
                uint8_t mov_cl[] = {0xB1, (uint8_t)shift_amount};
                buffer_append(b, mov_cl, 2);
                
                // SHL reg, CL
                uint8_t shl_cl[] = {0xD3, 0xE0};  // SHL reg, CL
                shl_cl[1] = 0xE0 + get_reg_index(reg);
                buffer_append(b, shl_cl, 2);
            }
            return;
        }
    }
    
    // If direct shifting doesn't work, try a different approach
    // Use byte-by-byte construction with shifts where possible
    generate_mov_reg_imm(b, insn);  // Fallback to standard approach
}

/*
 * Strategy definition
 */
strategy_t shift_value_construction_strategy = {
    .name = "Shift-Based Value Construction",
    .can_handle = can_handle_shift_value_construction,
    .get_size = get_size_shift_value_construction,
    .generate = generate_shift_value_construction,
    .priority = 78
};