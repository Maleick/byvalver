/*
 * BYVALVER - Incremental Byte Register Syscall (Priority 78)
 *
 * For sequential syscall numbers, uses INCB (increment byte) on specific
 * registers rather than repeated MOV instructions.
 * INCB encoding is always null-free (FE C3 for INCB BL).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"

// Track last seen value for sequential detection
static int last_reg = -1;
static int last_value = -1;

// ============================================================================
// Helper: Check if value is sequential increment
// ============================================================================

static int is_sequential_increment(uint8_t reg, int value) {
    if (last_reg == reg && last_value != -1) {
        if (value == last_value + 1) {
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// Strategy: Incremental Byte Register Syscall
// ============================================================================

int can_handle_incremental_byte_register_syscall(cs_insn *insn) {
    // Handle MOV byte_reg, imm for small syscall numbers (1-10)
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    // Must be byte register, immediate
    if (insn->detail->x86.operands[0].type != X86_OP_REG ||
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    uint8_t dest_reg = insn->detail->x86.operands[0].reg;
    int64_t imm = insn->detail->x86.operands[1].imm;

    // Only handle byte registers (AL, BL, CL, DL, AH, BH, CH, DH)
    if (dest_reg != X86_REG_AL && dest_reg != X86_REG_BL &&
        dest_reg != X86_REG_CL && dest_reg != X86_REG_DL &&
        dest_reg != X86_REG_AH && dest_reg != X86_REG_BH &&
        dest_reg != X86_REG_CH && dest_reg != X86_REG_DH) {
        return 0;
    }

    // Only handle small values (1-10)
    if (imm < 1 || imm > 10) {
        last_reg = -1;
        last_value = -1;
        return 0;
    }

    // Check if this is sequential
    int is_seq = is_sequential_increment(dest_reg, (int)imm);

    // Update tracking
    last_reg = dest_reg;
    last_value = (int)imm;

    // Only apply if sequential
    return is_seq;
}

size_t get_incremental_byte_register_syscall_size(cs_insn *insn) {
    (void)insn;
    // INCB reg (FE Cx) = 2 bytes
    return 2;
}

void generate_incremental_byte_register_syscall(struct buffer *b, cs_insn *insn) {
    uint8_t dest_reg = insn->detail->x86.operands[0].reg;

    // INCB reg encoding
    uint8_t inc_byte[] = {0xFE, 0x00};

    // Determine ModR/M byte based on register
    switch (dest_reg) {
        case X86_REG_AL: inc_byte[1] = 0xC0; break;
        case X86_REG_CL: inc_byte[1] = 0xC1; break;
        case X86_REG_DL: inc_byte[1] = 0xC2; break;
        case X86_REG_BL: inc_byte[1] = 0xC3; break;
        case X86_REG_AH: inc_byte[1] = 0xC4; break;
        case X86_REG_CH: inc_byte[1] = 0xC5; break;
        case X86_REG_DH: inc_byte[1] = 0xC6; break;
        case X86_REG_BH: inc_byte[1] = 0xC7; break;
        default:
            // Fallback: copy original
            buffer_append(b, insn->bytes, insn->size);
            return;
    }

    buffer_append(b, inc_byte, 2);
}

strategy_t incremental_byte_register_syscall_strategy = {
    .name = "Incremental Byte Register Syscall",
    .can_handle = can_handle_incremental_byte_register_syscall,
    .get_size = get_incremental_byte_register_syscall_size,
    .generate = generate_incremental_byte_register_syscall,
    .priority = 78
};

void register_incremental_byte_register_syscall_strategy() {
    register_strategy(&incremental_byte_register_syscall_strategy);
}
