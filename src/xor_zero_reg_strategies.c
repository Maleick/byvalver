#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 10: XOR reg, reg for Zeroing Registers
// Consistently use the `XOR` instruction with the same register to zero it out,
// which is a classic null-byte-free alternative to `MOV reg, 0`.

int can_handle_xor_reg_zeroing(cs_insn *insn) {
    // Check if this is a MOV instruction with immediate zero value
    if (insn->id == X86_INS_MOV) {
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *dst_op = &insn->detail->x86.operands[0];
            cs_x86_op *src_op = &insn->detail->x86.operands[1];

            // Check if destination is a register and source is immediate zero
            if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
                if (src_op->imm == 0) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

size_t get_size_xor_reg_zeroing(__attribute__((unused)) cs_insn *insn) {
    // XOR reg, reg is 2 bytes (same as MOV reg, 0 in some cases, but no null bytes)
    return 2; // Size for XOR reg, reg instruction
}

void generate_xor_reg_zeroing(struct buffer *b, cs_insn *insn) {
    // Convert MOV reg, 0 to XOR reg, reg
    if (insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];

        if (dst_op->type == X86_OP_REG) {
            x86_reg reg = dst_op->reg;

            // Generate XOR reg, reg instruction
            uint8_t xor_reg_reg[] = {0x31, 0x00};
            uint8_t reg_idx = get_reg_index(reg);
            xor_reg_reg[1] = (reg_idx << 3) | reg_idx; // ModR/M byte with reg as both operands
            buffer_append(b, xor_reg_reg, 2);
        } else {
            // Fallback if not in expected format
            buffer_append(b, insn->bytes, insn->size);
        }
    } else {
        buffer_append(b, insn->bytes, insn->size);
    }
}

// Alternative approach: PUSH 0, POP reg for register zeroing
int can_handle_push_pop_zero_reg(cs_insn *insn) {
    if (insn->id == X86_INS_MOV) {
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *dst_op = &insn->detail->x86.operands[0];
            cs_x86_op *src_op = &insn->detail->x86.operands[1];

            // Check if destination is a register and source is immediate zero
            if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
                if (src_op->imm == 0) {
                    x86_reg reg = dst_op->reg;
                    // Only consider for general purpose registers that can be used with PUSH/POP
                    if (reg >= X86_REG_EAX && reg <= X86_REG_EDI &&
                        reg != X86_REG_ESP && reg != X86_REG_EBP) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

size_t get_size_push_pop_zero_reg(__attribute__((unused)) cs_insn *insn) {
    // PUSH imm32 + POP reg = 5 + 1 = 6 bytes, but PUSH 0 with null bytes in immediate
    // would be problematic. We'd need PUSH with null-free construction which would be longer
    return 8; // Estimated size for alternative approach
}

void generate_push_pop_zero_reg(struct buffer *b, cs_insn *insn) {
    if (insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];

        if (dst_op->type == X86_OP_REG) {
            x86_reg reg = dst_op->reg;

            // Use XOR reg, reg (the simpler and more efficient approach)
            uint8_t xor_reg_reg[] = {0x31, 0x00};
            uint8_t reg_idx = get_reg_index(reg);
            xor_reg_reg[1] = (reg_idx << 3) | reg_idx; // ModR/M byte with reg as both operands
            buffer_append(b, xor_reg_reg, 2);
        } else {
            // Fallback if not in expected format
            buffer_append(b, insn->bytes, insn->size);
        }
    } else {
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t xor_reg_zeroing_strategy = {
    .name = "xor_reg_zeroing",
    .can_handle = can_handle_xor_reg_zeroing,
    .get_size = get_size_xor_reg_zeroing,
    .generate = generate_xor_reg_zeroing,
    .priority = 100  // Very high priority for register zeroing
};

strategy_t push_pop_zero_reg_strategy = {
    .name = "push_pop_zero_reg",
    .can_handle = can_handle_push_pop_zero_reg,
    .get_size = get_size_push_pop_zero_reg,
    .generate = generate_push_pop_zero_reg,
    .priority = 75  // Medium-high priority
};

// Register the XOR register zeroing strategies
void register_xor_zero_reg_strategies() {
    register_strategy(&xor_reg_zeroing_strategy);
    register_strategy(&push_pop_zero_reg_strategy);
}