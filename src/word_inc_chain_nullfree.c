/*
 * BYVALVER - Word-Size INC Chain Null-Free (Priority 77)
 *
 * When constructing small integer values (like syscall numbers 1-11),
 * uses chain of 16-bit INC operations on AX register instead of MOV AL.
 * 16-bit INC (66 40) never contains null bytes.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"

// ============================================================================
// Strategy: Word-Size INC Chain Null-Free
// ============================================================================

int can_handle_word_inc_chain_nullfree(cs_insn *insn) {
    // Handle MOV reg, imm for small values (1-10)
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    // Must be register, immediate
    if (insn->detail->x86.operands[0].type != X86_OP_REG ||
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;

    // Only handle small values 1-10 (optimal for INC chains)
    if (imm < 1 || imm > 10) {
        return 0;
    }

    // Check if destination is 8-bit or 16-bit register
    uint8_t dest_reg = insn->detail->x86.operands[0].reg;
    if (dest_reg != X86_REG_AL && dest_reg != X86_REG_AH &&
        dest_reg != X86_REG_BL && dest_reg != X86_REG_BH &&
        dest_reg != X86_REG_CL && dest_reg != X86_REG_CH &&
        dest_reg != X86_REG_DL && dest_reg != X86_REG_DH &&
        dest_reg != X86_REG_AX && dest_reg != X86_REG_BX &&
        dest_reg != X86_REG_CX && dest_reg != X86_REG_DX) {
        return 0;
    }

    return 1;
}

size_t get_word_inc_chain_nullfree_size(cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    // XOR AX, AX (3 bytes) + INC AX * imm (2 bytes each)
    return 3 + (imm * 2);
}

void generate_word_inc_chain_nullfree(struct buffer *b, cs_insn *insn) {
    uint8_t dest_reg = insn->detail->x86.operands[0].reg;
    uint32_t target = (uint32_t)insn->detail->x86.operands[1].imm;

    // Determine which 16-bit register to use based on destination
    uint8_t reg16 = X86_REG_AX;
    if (dest_reg == X86_REG_BL || dest_reg == X86_REG_BH || dest_reg == X86_REG_BX) {
        reg16 = X86_REG_BX;
    } else if (dest_reg == X86_REG_CL || dest_reg == X86_REG_CH || dest_reg == X86_REG_CX) {
        reg16 = X86_REG_CX;
    } else if (dest_reg == X86_REG_DL || dest_reg == X86_REG_DH || dest_reg == X86_REG_DX) {
        reg16 = X86_REG_DX;
    }

    uint8_t reg16_idx = get_reg_index(reg16);

    // XOR reg16, reg16 (66 31 C0+r)
    uint8_t xor_bytes[] = {0x66, 0x31, (uint8_t)(0xC0 + (reg16_idx << 3) + reg16_idx)};
    buffer_append(b, xor_bytes, 3);

    // INC reg16 repeatedly (66 40+r)
    for (uint32_t i = 0; i < target; i++) {
        uint8_t inc_bytes[] = {0x66, (uint8_t)(0x40 + reg16_idx)};
        buffer_append(b, inc_bytes, 2);
    }
}

strategy_t word_inc_chain_nullfree_strategy = {
    .name = "Word-Size INC Chain Null-Free",
    .can_handle = can_handle_word_inc_chain_nullfree,
    .get_size = get_word_inc_chain_nullfree_size,
    .generate = generate_word_inc_chain_nullfree,
    .priority = 77
};

void register_word_inc_chain_nullfree_strategy() {
    register_strategy(&word_inc_chain_nullfree_strategy);
}
