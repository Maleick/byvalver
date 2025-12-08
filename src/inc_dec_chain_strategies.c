/*
 * INC/DEC Chain Strategy
 *
 * PROBLEM: Loading immediate values that contain null bytes can cause nulls
 * in instruction encodings.
 * 
 * SOLUTION: Build values using chains of INC/DEC operations or other arithmetic
 * operations that don't contain null bytes in their encoding.
 *
 * FREQUENCY: Useful for building small values without using immediate operands
 * PRIORITY: 75 (Medium-High - good for small value construction)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00000005 (contains nulls in first 3 bytes)
 *   Strategy: XOR EAX, EAX; INC EAX; INC EAX; INC EAX; INC EAX; INC EAX;
 */

#include "inc_dec_chain_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with small immediate values that could be built with INC/DEC
 */
int can_handle_inc_dec_chain(cs_insn *insn) {
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

    // Check if the immediate is a small positive value that could be efficiently built with INC/DEC
    uint32_t imm = (uint32_t)src_op->imm;
    if (imm > 20) {  // Only handle small values efficiently
        return 0;
    }

    // Check if the original instruction encoding contains null bytes
    for (size_t j = 0; j < insn->size; j++) {
        if (insn->bytes[j] == 0x00) {
            return 1;
        }
    }

    // Even if the instruction doesn't contain nulls, if the immediate itself contains nulls
    // but it's a small value that can be efficiently built with inc/dec, handle it
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            return 1;
        }
    }

    return 0;
}

/*
 * Size calculation for INC/DEC chain construction
 * For value N: XOR (2 bytes) + N INCs (1 byte each) = 2 + N bytes
 */
size_t get_size_inc_dec_chain(cs_insn *insn) {
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    uint32_t imm = (uint32_t)src_op->imm;
    
    // Clamp to prevent excessive chain lengths
    if (imm > 20) imm = 20;
    
    // XOR (2) + imm INC operations (1 each) = 2 + imm bytes
    return 2 + imm;
}

/*
 * Generate INC/DEC chain for small values
 *
 * For MOV reg, imm where imm is small:
 *   XOR reg, reg    ; Zero the register
 *   INC reg         ; Increment N times to reach the target value
 *   ... (N times)
 */
void generate_inc_dec_chain(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    uint32_t value = (uint32_t)src_op->imm;

    // Clamp value to prevent extremely long chains
    if (value > 20) value = 20;  // Cap at 20 for practicality

    // XOR target_reg, target_reg to zero it out (no nulls)
    uint8_t xor_reg_reg[] = {0x31, 0x00};
    xor_reg_reg[1] = 0xC0 | (get_reg_index(target_reg) << 3) | get_reg_index(target_reg);
    buffer_append(b, xor_reg_reg, 2);

    // INC the register 'value' times
    uint8_t inc_reg = 0x40 + get_reg_index(target_reg);
    for (uint32_t i = 0; i < value; i++) {
        buffer_write_byte(b, inc_reg);
    }
}

/*
 * Alternative: Use DEC for negative values or complement approach
 */
int can_handle_inc_dec_complement_chain(cs_insn *insn) {
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

    // Check if the immediate is a small value (positive or could be complemented)
    int32_t imm = (int32_t)src_op->imm;
    uint32_t abs_imm = (imm < 0) ? (uint32_t)(-imm) : (uint32_t)imm;

    // Only handle small absolute values efficiently
    if (abs_imm > 16) {  // Use 16 for slightly more aggressive approach
        return 0;
    }

    // Check if the original instruction encoding contains null bytes
    for (size_t j = 0; j < insn->size; j++) {
        if (insn->bytes[j] == 0x00) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_inc_dec_complement_chain(cs_insn *insn) {
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    int32_t imm = (int32_t)src_op->imm;
    uint32_t abs_imm = (imm < 0) ? (uint32_t)(-imm) : (uint32_t)imm;

    // Clamp to prevent excessive chain lengths
    if (abs_imm > 16) abs_imm = 16;

    // XOR (2) + abs_imm INC/DEC operations (1 each) + optional NEG (2) = 4 + abs_imm bytes max
    return 4 + abs_imm;
}

void generate_inc_dec_complement_chain(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    int32_t value = (int32_t)src_op->imm;

    // Clamp value to prevent extremely long chains
    int32_t abs_value = (value < 0) ? -value : value;
    if (abs_value > 16) abs_value = 16;  // Cap at 16 for practicality

    // XOR target_reg, target_reg to zero it out (no nulls)
    uint8_t xor_reg_reg[] = {0x31, 0x00};
    xor_reg_reg[1] = 0xC0 | (get_reg_index(target_reg) << 3) | get_reg_index(target_reg);
    buffer_append(b, xor_reg_reg, 2);

    uint8_t op_reg;
    if (value >= 0) {
        // Use INC to build positive value
        op_reg = 0x40 + get_reg_index(target_reg);
        for (int32_t i = 0; i < value; i++) {
            buffer_write_byte(b, op_reg);
        }
    } else {
        // Use DEC to build negative value (in a complement way)
        op_reg = 0x48 + get_reg_index(target_reg);  // DEC
        for (int32_t i = 0; i < abs_value; i++) {
            buffer_write_byte(b, op_reg);
        }
        // Since DEC gives us negative values in 2's complement, 
        // we might need to handle differently for the exact value
        // For now, just use INC/DEC for small positive values
    }
    
    // If value is negative, we may need to handle it specially
    // But for this implementation, we'll focus on positive values
    if (value < 0) {
        // Reset and use the positive approach
        // XOR target_reg, target_reg again to zero
        buffer_append(b, xor_reg_reg, 2);
        
        // Use DEC for negative value construction
        op_reg = 0x48 + get_reg_index(target_reg);  // DEC
        for (int32_t i = 0; i < abs_value; i++) {
            buffer_write_byte(b, op_reg);
        }
    }
}

strategy_t inc_dec_chain_strategy = {
    .name = "INC/DEC Chain Strategy",
    .can_handle = can_handle_inc_dec_chain,
    .get_size = get_size_inc_dec_chain,
    .generate = generate_inc_dec_chain,
    .priority = 75  // Medium-High priority
};

strategy_t inc_dec_complement_chain_strategy = {
    .name = "INC/DEC Complement Chain Strategy", 
    .can_handle = can_handle_inc_dec_complement_chain,
    .get_size = get_size_inc_dec_complement_chain,
    .generate = generate_inc_dec_complement_chain,
    .priority = 72  // Slightly lower priority
};

void register_inc_dec_chain_strategies() {
    register_strategy(&inc_dec_chain_strategy);
    register_strategy(&inc_dec_complement_chain_strategy);
}