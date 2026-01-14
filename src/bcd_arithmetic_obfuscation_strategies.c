/*
 * BCD Arithmetic for Obfuscated Constant Generation Strategy
 *
 * PROBLEM: MOV immediate instructions may contain bad bytes in the immediate value.
 *
 * SOLUTION: Use BCD arithmetic instructions (AAM, AAD, DAA, DAS) to construct
 * constant values through arithmetic operations, providing obfuscation and bad-byte avoidance.
 *
 * Example:
 * Original: mov al, 0x2A  (42 decimal, may have bad chars)
 * Transform: mov al, 2; mov ah, 4; aad  (AAD: AL = AH*10 + AL = 4*10 + 2 = 42)
 */

#include "bcd_arithmetic_obfuscation_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Check if this strategy can handle the instruction
 */
int can_handle_bcd_arithmetic(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Only handle MOV instructions with immediate to AL register
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    // Check if it's MOV al, imm8
    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    // Destination must be AL register
    if (dst->type != X86_OP_REG || dst->reg != X86_REG_AL) {
        return 0;
    }

    // Source must be immediate
    if (src->type != X86_OP_IMM) {
        return 0;
    }

    // Get the immediate value
    uint64_t imm = src->imm;

    // Only handle values 0-99 (practical range for BCD decomposition)
    if (imm > 99) {
        return 0;
    }

    // Check if the instruction encoding contains bad bytes
    if (!is_bad_byte_free_buffer(insn->bytes, insn->size)) {
        return 1;  // Has bad chars, we can handle it
    }

    return 0;  // No bad chars, no need to transform
}

/**
 * Calculate size of transformed instruction
 */
size_t get_size_bcd_arithmetic(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Transformation: mov al, ones; mov ah, tens; aad
    // MOV AL, imm8 = 2 bytes (B0 XX)
    // MOV AH, imm8 = 2 bytes (B4 XX)
    // AAD = 2 bytes (D5 0A)
    // Total = 6 bytes
    return 6;
}

/**
 * Generate transformed instruction sequence
 */
void generate_bcd_arithmetic(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Get the immediate value
    if (insn->detail->x86.op_count != 2) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cs_x86_op *src = &insn->detail->x86.operands[1];
    if (src->type != X86_OP_IMM) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    uint8_t value = (uint8_t)src->imm;

    // Decompose value into tens and ones using BCD
    // value = tens * 10 + ones
    uint8_t tens = value / 10;
    uint8_t ones = value % 10;

    // Generate: mov al, ones; mov ah, tens; aad
    // AAD will compute: AL = AH * 10 + AL

    // MOV AL, ones (B0 XX)
    buffer_write_byte(b, 0xB0);
    buffer_write_byte(b, ones);

    // MOV AH, tens (B4 XX)
    buffer_write_byte(b, 0xB4);
    buffer_write_byte(b, tens);

    // AAD (D5 0A) - ASCII Adjust AX Before Division
    // This instruction multiplies AH by 10 and adds AL: AL = AH*10 + AL
    buffer_write_byte(b, 0xD5);
    buffer_write_byte(b, 0x0A);
}

// Define the strategy structure
strategy_t bcd_arithmetic_strategy = {
    .name = "BCD Arithmetic Obfuscation",
    .can_handle = can_handle_bcd_arithmetic,
    .get_size = get_size_bcd_arithmetic,
    .generate = generate_bcd_arithmetic,
    .priority = 68,
    .target_arch = BYVAL_ARCH_X86
};
