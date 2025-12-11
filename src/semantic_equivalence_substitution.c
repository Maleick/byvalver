/*
 * BYVALVER - Semantic Equivalence Substitution (Priority 88)
 *
 * Replaces common instructions with rare but functionally equivalent sequences.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"
#include "obfuscation_strategy_registry.h"

// ============================================================================
// Strategy 1: XOR reg, reg → SUB reg, reg
// ============================================================================

int can_handle_xor_to_sub(cs_insn *insn) {
    if (insn->id != X86_INS_XOR) return 0;

    if (insn->detail->x86.op_count == 2) {
        cs_x86_op *op1 = &insn->detail->x86.operands[0];
        cs_x86_op *op2 = &insn->detail->x86.operands[1];

        // XOR reg, reg (same register)
        if (op1->type == X86_OP_REG && op2->type == X86_OP_REG) {
            return (op1->reg == op2->reg);
        }
    }
    return 0;
}

size_t get_xor_to_sub_size(cs_insn *insn) {
    (void)insn;
    return 2;  // SUB reg, reg (2 bytes)
}

void generate_xor_to_sub(struct buffer *b, cs_insn *insn) {
    uint8_t reg = insn->detail->x86.operands[0].reg;
    uint8_t reg_idx = get_reg_index(reg);

    // SUB reg, reg (functionally equivalent to XOR reg, reg)
    uint8_t bytes[] = {0x29, (uint8_t)(0xC0 | (reg_idx << 3) | reg_idx)};
    buffer_append(b, bytes, 2);
}

static strategy_t xor_to_sub_strategy = {
    .name = "XOR→SUB Equivalence",
    .can_handle = can_handle_xor_to_sub,
    .get_size = get_xor_to_sub_size,
    .generate = generate_xor_to_sub,
    .priority = 88
};

// ============================================================================
// Strategy 2: INC reg → ADD reg, 1
// ============================================================================

int can_handle_inc_to_add(cs_insn *insn) {
    if (insn->id != X86_INS_INC) return 0;

    if (insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        // Only handle INC reg
        return (op->type == X86_OP_REG);
    }
    return 0;
}

size_t get_inc_to_add_size(cs_insn *insn) {
    (void)insn;
    return 3;  // ADD reg, 1 (opcode + ModR/M + imm8)
}

void generate_inc_to_add(struct buffer *b, cs_insn *insn) {
    uint8_t reg = insn->detail->x86.operands[0].reg;
    uint8_t reg_idx = get_reg_index(reg);

    // ADD reg, 1
    uint8_t bytes[] = {0x83, (uint8_t)(0xC0 | reg_idx), 0x01};
    buffer_append(b, bytes, 3);
}

static strategy_t inc_to_add_strategy = {
    .name = "INC→ADD Equivalence",
    .can_handle = can_handle_inc_to_add,
    .get_size = get_inc_to_add_size,
    .generate = generate_inc_to_add,
    .priority = 87
};

// ============================================================================
// Strategy 3: DEC reg → SUB reg, 1
// ============================================================================

int can_handle_dec_to_sub(cs_insn *insn) {
    if (insn->id != X86_INS_DEC) return 0;

    if (insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        return (op->type == X86_OP_REG);
    }
    return 0;
}

size_t get_dec_to_sub_size(cs_insn *insn) {
    (void)insn;
    return 3;  // SUB reg, 1
}

void generate_dec_to_sub(struct buffer *b, cs_insn *insn) {
    uint8_t reg = insn->detail->x86.operands[0].reg;
    uint8_t reg_idx = get_reg_index(reg);

    // SUB reg, 1
    uint8_t bytes[] = {0x83, (uint8_t)(0xE8 | reg_idx), 0x01};
    buffer_append(b, bytes, 3);
}

static strategy_t dec_to_sub_strategy = {
    .name = "DEC→SUB Equivalence",
    .can_handle = can_handle_dec_to_sub,
    .get_size = get_dec_to_sub_size,
    .generate = generate_dec_to_sub,
    .priority = 86
};

// ============================================================================
// Strategy 4: MOV reg, 0 → XOR reg, reg
// ============================================================================

int can_handle_mov_zero_to_xor(cs_insn *insn) {
    if (insn->id != X86_INS_MOV) return 0;

    if (insn->detail->x86.op_count == 2) {
        cs_x86_op *dst = &insn->detail->x86.operands[0];
        cs_x86_op *src = &insn->detail->x86.operands[1];

        // MOV reg, 0
        if (dst->type == X86_OP_REG && src->type == X86_OP_IMM) {
            return (src->imm == 0);
        }
    }
    return 0;
}

size_t get_mov_zero_to_xor_size(cs_insn *insn) {
    (void)insn;
    return 2;  // XOR reg, reg
}

void generate_mov_zero_to_xor(struct buffer *b, cs_insn *insn) {
    uint8_t reg = insn->detail->x86.operands[0].reg;
    uint8_t reg_idx = get_reg_index(reg);

    // XOR reg, reg
    uint8_t bytes[] = {0x31, (uint8_t)(0xC0 | (reg_idx << 3) | reg_idx)};
    buffer_append(b, bytes, 2);
}

static strategy_t mov_zero_to_xor_strategy = {
    .name = "MOV 0→XOR Equivalence",
    .can_handle = can_handle_mov_zero_to_xor,
    .get_size = get_mov_zero_to_xor_size,
    .generate = generate_mov_zero_to_xor,
    .priority = 85
};

// ============================================================================
// Strategy 5: INC reg → LEA reg, [reg+1]
// ============================================================================

int can_handle_inc_to_lea(cs_insn *insn) {
    if (insn->id != X86_INS_INC) return 0;

    if (insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        // Only handle 32-bit registers (LEA limitations)
        if (op->type == X86_OP_REG) {
            return (op->reg >= X86_REG_EAX && op->reg <= X86_REG_EDI);
        }
    }
    return 0;
}

size_t get_inc_to_lea_size(cs_insn *insn) {
    (void)insn;
    return 3;  // LEA reg, [reg+disp8]
}

void generate_inc_to_lea(struct buffer *b, cs_insn *insn) {
    uint8_t reg = insn->detail->x86.operands[0].reg;
    uint8_t reg_idx = get_reg_index(reg);

    // LEA reg, [reg+1]
    // Opcode: 8D, ModR/M: 01 reg reg (mod=01, reg=reg, rm=reg), disp8: 01
    uint8_t bytes[] = {0x8D, (uint8_t)(0x40 | (reg_idx << 3) | reg_idx), 0x01};
    buffer_append(b, bytes, 3);
}

static strategy_t inc_to_lea_strategy = {
    .name = "INC→LEA Equivalence",
    .can_handle = can_handle_inc_to_lea,
    .get_size = get_inc_to_lea_size,
    .generate = generate_inc_to_lea,
    .priority = 84
};

// ============================================================================
// Registration
// ============================================================================

void register_semantic_equivalence_substitution() {
    register_obfuscation_strategy(&xor_to_sub_strategy);
    register_obfuscation_strategy(&inc_to_add_strategy);
    register_obfuscation_strategy(&dec_to_sub_strategy);
    register_obfuscation_strategy(&mov_zero_to_xor_strategy);
    register_obfuscation_strategy(&inc_to_lea_strategy);
}
