/*
 * BYVALVER - JCXZ Null-Safe Loop Termination (Priority 86)
 *
 * JCXZ instruction (Jump if CX is Zero) provides null-free loop termination
 * that avoids CMP instructions with zero immediates.
 * JCXZ tests if ECX equals zero and jumps if true, all in 2 bytes (E3 XX).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"

// ============================================================================
// Helper: Check if next instruction is conditional jump
// ============================================================================

static int is_zero_comparison(cs_insn *insn) {
    // Check for CMP ECX, 0 or CMP CX, 0 or TEST ECX, ECX
    if (insn->id == X86_INS_CMP) {
        if (insn->detail->x86.op_count == 2) {
            // Check if comparing ECX/CX with 0
            if (insn->detail->x86.operands[0].type == X86_OP_REG &&
                (insn->detail->x86.operands[0].reg == X86_REG_ECX ||
                 insn->detail->x86.operands[0].reg == X86_REG_CX)) {
                if (insn->detail->x86.operands[1].type == X86_OP_IMM &&
                    insn->detail->x86.operands[1].imm == 0) {
                    return 1;
                }
            }
        }
    }

    if (insn->id == X86_INS_TEST) {
        if (insn->detail->x86.op_count == 2) {
            // TEST ECX, ECX or TEST CX, CX
            if (insn->detail->x86.operands[0].type == X86_OP_REG &&
                insn->detail->x86.operands[1].type == X86_OP_REG) {
                if ((insn->detail->x86.operands[0].reg == X86_REG_ECX &&
                     insn->detail->x86.operands[1].reg == X86_REG_ECX) ||
                    (insn->detail->x86.operands[0].reg == X86_REG_CX &&
                     insn->detail->x86.operands[1].reg == X86_REG_CX)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// ============================================================================
// Strategy: JCXZ Null-Safe Loop Termination
// ============================================================================

int can_handle_jcxz_null_safe_loop_termination(cs_insn *insn) {
    // Handle CMP ECX, 0 or TEST ECX, ECX patterns
    if (!is_zero_comparison(insn)) {
        return 0;
    }

    // Check if instruction contains null bytes
    if (!has_null_bytes(insn)) {
        return 0;
    }

    return 1;
}

size_t get_jcxz_null_safe_loop_termination_size(cs_insn *insn) {
    (void)insn;
    // JCXZ rel8 (2 bytes: E3 XX) - but we just replace the comparison
    // The actual jump needs to be handled separately, so we return size for NOP pattern
    return 2;
}

void generate_jcxz_null_safe_loop_termination(struct buffer *b, cs_insn *insn) {
    (void)insn;

    // We can't directly replace CMP with JCXZ because JCXZ includes the jump
    // Instead, we replace the CMP ECX, 0 with a null-free equivalent: TEST ECX, ECX
    // TEST ECX, ECX (85 C9) - null-free, 2 bytes
    uint8_t test_ecx[] = {0x85, 0xC9};
    buffer_append(b, test_ecx, 2);
}

strategy_t jcxz_null_safe_loop_termination_strategy = {
    .name = "JCXZ Null-Safe Loop Termination",
    .can_handle = can_handle_jcxz_null_safe_loop_termination,
    .get_size = get_jcxz_null_safe_loop_termination_size,
    .generate = generate_jcxz_null_safe_loop_termination,
    .priority = 86
};

void register_jcxz_null_safe_loop_termination_strategy() {
    register_strategy(&jcxz_null_safe_loop_termination_strategy);
}
