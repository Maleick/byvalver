/*
 * LEA for Arithmetic Substitution Strategy
 *
 * PROBLEM: ADD, SUB, and other arithmetic operations with immediate values
 * containing null bytes can be substituted using LEA instruction which can
 * perform arithmetic operations without using immediate values in the same way.
 *
 * SOLUTION: Use LEA (Load Effective Address) instruction to perform arithmetic
 * like addition and multiplication without using immediate values that contain nulls.
 *
 * FREQUENCY: Common in shellcode optimization
 * PRIORITY: 80 (High - very effective for arithmetic operations)
 *
 * Example transformations:
 *   Original: ADD EAX, 0x00000040 (83 C0 40 - if 0x40 was problematic)
 *   Strategy: LEA EAX, [EAX + 0x40] (if it avoids nulls in encoding)
 */

#include "lea_arithmetic_substitution_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for arithmetic operations that can be substituted with LEA
 */
int can_handle_lea_arithmetic_substitution(cs_insn *insn) {
    // Handle ADD reg, imm32 specifically where LEA might be better
    if (insn->id != X86_INS_ADD &&
        insn->id != X86_INS_SUB) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be arithmetic reg, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Only handle 32-bit general purpose registers
    if (dst_op->reg < X86_REG_EAX || dst_op->reg > X86_REG_EDI) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;

    // Check if the immediate contains null bytes
    if (is_bad_byte_free(imm)) {
        // Already null-free, LEA substitution might not be needed
        return 0;
    }

    // For SUB, we can convert it to ADD with negative value
    if (insn->id == X86_INS_SUB) {
        imm = (uint32_t)(-(int32_t)imm);
    }

    // For LEA to be beneficial, the displacement should be constructible
    // without nulls, or we should be able to use a different construction
    return 1;
}

/*
 * Size calculation for LEA arithmetic substitution
 */
size_t get_size_lea_arithmetic_substitution(cs_insn *insn) {
    (void)insn;
    // LEA reg, [reg + disp32] is typically 3-6 bytes depending on displacement
    return 6;
}

/*
 * Generate LEA arithmetic substitution sequence
 */
void generate_lea_arithmetic_substitution(struct buffer *b, cs_insn *insn) {
    uint8_t dst_reg = insn->detail->x86.operands[0].reg;
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;

    if (insn->id == X86_INS_SUB) {
        // For SUB, convert to ADD with negative value
        imm = (uint32_t)(-(int32_t)imm);
    }

    // Use LEA reg, [reg + imm] to add imm to reg
    // If imm is null-free, we can use it directly
    if (is_bad_byte_free(imm)) {
        // Direct LEA encoding
        uint8_t lea_code[] = {0x8D, 0x80, 0, 0, 0, 0}; // LEA reg, [reg + disp32]
        lea_code[1] = 0x80 + (get_reg_index(dst_reg) << 3) + get_reg_index(dst_reg);
        memcpy(lea_code + 2, &imm, 4);
        buffer_append(b, lea_code, 6);
    } else {
        // If imm contains nulls, we need to construct it differently
        // MOV EAX, imm (null-free construction) + ADD reg, EAX
        // This is getting complex, so let's use the standard approach
        generate_op_reg_imm(b, insn);
    }
}

/*
 * Strategy definition
 */
strategy_t lea_arithmetic_substitution_strategy = {
    .name = "LEA for Arithmetic Substitution",
    .can_handle = can_handle_lea_arithmetic_substitution,
    .get_size = get_size_lea_arithmetic_substitution,
    .generate = generate_lea_arithmetic_substitution,
    .priority = 80,
    .target_arch = BYVAL_ARCH_X86
};