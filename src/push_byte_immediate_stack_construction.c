/*
 * BYVALVER - PUSH Byte Immediate Stack Construction (Priority 82)
 *
 * Uses PUSH with 8-bit sign-extended immediates for small values instead of
 * PUSH with full 32-bit immediates. For values -128 to +127, PUSH byte is
 * always null-free and compact (2 bytes vs 5 bytes).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"

// ============================================================================
// Strategy: PUSH Byte Immediate Stack Construction
// ============================================================================

int can_handle_push_byte_immediate_stack_construction(cs_insn *insn) {
    // Handle PUSH imm32 where value fits in signed byte (-128 to +127)
    if (insn->id != X86_INS_PUSH) {
        return 0;
    }

    if (insn->detail->x86.op_count != 1) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_IMM) {
        return 0;
    }

    int64_t imm = insn->detail->x86.operands[0].imm;

    // Check if value fits in signed byte (-128 to +127)
    if (imm < -128 || imm > 127) {
        return 0;
    }

    // Check if original instruction has null bytes
    if (!has_null_bytes(insn)) {
        return 0;
    }

    return 1;
}

size_t get_push_byte_immediate_stack_construction_size(cs_insn *insn) {
    (void)insn;
    // PUSH byte (6A XX) = 2 bytes
    return 2;
}

void generate_push_byte_immediate_stack_construction(struct buffer *b, cs_insn *insn) {
    int64_t imm = insn->detail->x86.operands[0].imm;

    // PUSH byte imm8 (6A + byte)
    uint8_t push_byte[] = {0x6A, (uint8_t)(imm & 0xFF)};
    buffer_append(b, push_byte, 2);
}

strategy_t push_byte_immediate_stack_construction_strategy = {
    .name = "PUSH Byte Immediate Stack Construction",
    .can_handle = can_handle_push_byte_immediate_stack_construction,
    .get_size = get_push_byte_immediate_stack_construction_size,
    .generate = generate_push_byte_immediate_stack_construction,
    .priority = 82
};

void register_push_byte_immediate_stack_construction_strategy() {
    register_strategy(&push_byte_immediate_stack_construction_strategy);
}
