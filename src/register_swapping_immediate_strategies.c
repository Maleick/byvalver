/*
 * Register Swapping with Immediate Loading Strategy
 *
 * PROBLEM: Loading immediate values that contain null bytes into registers
 * especially when other registers might have useful values.
 *
 * SOLUTION: Use register exchange operations (XCHG) to load immediate values
 * by first loading null-free partial values and then exchanging them.
 *
 * FREQUENCY: Useful in contexts where register state can be leveraged
 * PRIORITY: 70 (Medium - situationally useful)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00123456 (B8 56 34 12 00 - contains null)
 *   Strategy: Use XCHG with other registers that have pre-loaded values
 */

#include "register_swapping_immediate_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV reg, imm32 that could benefit from register swapping
 */
int can_handle_register_swapping_immediate(cs_insn *insn) {
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

    // This strategy might be beneficial in specific contexts
    // For now, return 0 since it needs context about other register states
    return 0;
}

/*
 * Size calculation for register swapping with immediate loading
 */
size_t get_size_register_swapping_immediate(cs_insn *insn) {
    (void)insn;
    // PUSH + MOV + XCHG + POP = ~6-8 bytes typically
    return 8;
}

/*
 * Generate register swapping with immediate loading sequence
 */
void generate_register_swapping_immediate(struct buffer *b, cs_insn *insn) {
    // This is context-dependent and complex to implement generically
    // For now, fallback to standard null-free MOV
    generate_mov_reg_imm(b, insn);
}

/*
 * Strategy definition
 */
strategy_t register_swapping_immediate_strategy = {
    .name = "Register Swapping with Immediate Loading",
    .can_handle = can_handle_register_swapping_immediate,
    .get_size = get_size_register_swapping_immediate,
    .generate = generate_register_swapping_immediate,
    .priority = 70
};