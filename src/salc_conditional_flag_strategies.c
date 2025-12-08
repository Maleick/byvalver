/*
 * SALC + Conditional Flag Manipulation Strategy
 *
 * PROBLEM: Setting AL register to 0 or other values that might contain nulls
 * in MOV instructions. Also manipulating flags without operations that contain nulls.
 *
 * SOLUTION: Use SALC (Set AL on Carry) instruction combined with flag manipulation
 * to set AL register efficiently, and use alternative flag-preserving operations.
 *
 * FREQUENCY: Common in 32-bit shellcode for efficient zeroing without nulls
 * PRIORITY: 91 (Very High - efficient and stealthy approach)
 *
 * Example transformations:
 *   Original: MOV AL, 0x00 (B0 00 - contains null)
 *   Strategy: CLC; SALC (F8 D6 - no nulls)
 */

#include "salc_conditional_flag_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV AL, imm8 where imm8 is 0 or other patterns
 */
int can_handle_salc_conditional_flag(cs_insn *insn) {
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

    // Must be MOV AL specifically
    if (dst_op->reg != X86_REG_AL) {
        return 0;
    }

    uint8_t imm = (uint8_t)src_op->imm;

    // SALC can set AL to 0x00 (CF=0) or 0xFF (CF=1)
    if (imm != 0x00 && imm != 0xFF) {
        return 0;
    }

    return 1;
}

/*
 * Size calculation for SALC + flag manipulation
 */
size_t get_size_salc_conditional_flag(cs_insn *insn) {
    (void)insn;
    // CLC (1) + SALC (1) = 2 bytes for zeroing AL
    // STC (1) + SALC (1) = 2 bytes for setting AL to 0xFF
    return 2;
}

/*
 * Generate SALC + flag manipulation sequence
 */
void generate_salc_conditional_flag(struct buffer *b, cs_insn *insn) {
    uint8_t target_value = (uint8_t)insn->detail->x86.operands[1].imm;

    if (target_value == 0x00) {
        // CLC - Clear carry flag
        buffer_write_byte(b, 0xF8);
        // SALC - Set AL based on carry (AL = 0x00 when CF=0)
        buffer_write_byte(b, 0xD6);
    } else if (target_value == 0xFF) {
        // STC - Set carry flag
        buffer_write_byte(b, 0xF9);
        // SALC - Set AL based on carry (AL = 0xFF when CF=1)
        buffer_write_byte(b, 0xD6);
    }
}

/*
 * Strategy definition
 */
strategy_t salc_conditional_flag_strategy = {
    .name = "SALC + Conditional Flag Manipulation",
    .can_handle = can_handle_salc_conditional_flag,
    .get_size = get_size_salc_conditional_flag,
    .generate = generate_salc_conditional_flag,
    .priority = 91
};