/*
 * LEA-based Address Calculation Strategy
 *
 * PROBLEM: Arithmetic operations with immediate values containing null bytes.
 * 
 * SOLUTION: Use LEA instruction to perform arithmetic operations (addition,
 * multiplication by 1, 2, 4, or 8) without using immediate operands that 
 * contain null bytes.
 *
 * FREQUENCY: Useful for arithmetic operations without null bytes in results
 * PRIORITY: 78 (High - efficient for arithmetic without immediate nulls)
 *
 * Example transformations:
 *   Original: ADD EAX, 0x00001000 (contains nulls)
 *   Strategy: LEA EAX, [EAX + 0x1000] (if 0x1000 is null-free)
 */

#include "lea_arithmetic_calculation_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for ADD operations with immediate values containing null bytes
 */
int can_handle_lea_arithmetic_add(cs_insn *insn) {
    if (insn->id != X86_INS_ADD ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be ADD register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Check if the immediate contains null bytes
    uint32_t imm = (uint32_t)src_op->imm;
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            // Check if the original instruction encoding contains nulls too
            for (size_t j = 0; j < insn->size; j++) {
                if (insn->bytes[j] == 0x00) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*
 * Size calculation for LEA arithmetic addition
 * LEA reg, [reg + disp32] is typically 6 bytes
 */
size_t get_size_lea_arithmetic_add(cs_insn *insn) {
    (void)insn; // Unused parameter
    return 6; // LEA reg, [reg + disp32]
}

/*
 * Generate LEA-based arithmetic addition
 *
 * For ADD reg, imm32 that contains null bytes in immediate:
 *   Use LEA reg, [reg + imm32] if imm32 is null-free
 *   Otherwise, use alternative method
 */
void generate_lea_arithmetic_add(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    uint32_t value_to_add = (uint32_t)src_op->imm;

    // Check if the value to add is null-free - if so, we can use LEA directly
    if (is_bad_char_free(value_to_add)) {
        // Use LEA target_reg, [target_reg + value_to_add]
        // This performs: target_reg = target_reg + value_to_add
        uint8_t lea_code[] = {0x8D, 0x00, 0x00, 0x00, 0x00, 0x00};
        // ModR/M for [reg + disp32] format: 00 rrr rrrr where rrr is target reg
        lea_code[1] = 0x80 | get_reg_index(target_reg); // 80-87 for disp32 forms
        lea_code[2] = (value_to_add >> 0) & 0xFF;
        lea_code[3] = (value_to_add >> 8) & 0xFF;
        lea_code[4] = (value_to_add >> 16) & 0xFF;
        lea_code[5] = (value_to_add >> 24) & 0xFF;
        buffer_append(b, lea_code, 6);
    } else {
        // If the immediate contains nulls, we can't use LEA with that displacement directly
        // We'll need to build the value using other methods first
        // For now, use the general null-free approach
        generate_mov_eax_imm(b, value_to_add);
        
        // If target_reg is not EAX, move it
        if (target_reg != X86_REG_EAX) {
            uint8_t push_target = 0x50 + get_reg_index(target_reg);
            buffer_write_byte(b, push_target); // Save original value
            
            uint8_t mov_target_eax[] = {0x89, 0xC0};
            mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
            buffer_append(b, mov_target_eax, 2);
            
            uint8_t pop_target = 0x58 + get_reg_index(target_reg);
            buffer_write_byte(b, pop_target); // Restore original value
        } else {
            // EAX already has the value, so no further operation needed
        }
    }
}

/*
 * Another form: LEA for multiplication and addition
 */
int can_handle_lea_multiplication_addition(cs_insn *insn) {
    // Handle ADD, SUB, MOV operations that could benefit from LEA's arithmetic
    if ((insn->id != X86_INS_ADD && insn->id != X86_INS_SUB && insn->id != X86_INS_MOV) ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be operation with register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Check if the immediate contains null bytes
    uint32_t imm = (uint32_t)src_op->imm;
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            // Check if the original instruction encoding contains nulls too
            for (size_t j = 0; j < insn->size; j++) {
                if (insn->bytes[j] == 0x00) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

size_t get_size_lea_multiplication_addition(cs_insn *insn) {
    (void)insn; // Unused parameter
    return 6; // LEA reg, [reg + disp32]
}

void generate_lea_multiplication_addition(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    uint32_t value = (uint32_t)src_op->imm;

    // Use LEA for addition if the displacement is null-free
    if (is_bad_char_free(value)) {
        // Use LEA target_reg, [target_reg + value] for ADD-like behavior
        uint8_t lea_code[] = {0x8D, 0x80, 0x00, 0x00, 0x00, 0x00};
        lea_code[1] = lea_code[1] | get_reg_index(target_reg); // ModR/M with disp32
        lea_code[2] = (value >> 0) & 0xFF;
        lea_code[3] = (value >> 8) & 0xFF;
        lea_code[4] = (value >> 16) & 0xFF;
        lea_code[5] = (value >> 24) & 0xFF;
        buffer_append(b, lea_code, 6);
    } else {
        // Fallback to standard null-free approach
        generate_mov_eax_imm(b, value);
        
        if (insn->id == X86_INS_ADD) {
            // If original was ADD, we need to add the value to the register
            if (target_reg != X86_REG_EAX) {
                uint8_t add_target_eax[] = {0x01, 0xC0};
                add_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
                buffer_append(b, add_target_eax, 2);
            }
        } else if (insn->id == X86_INS_MOV) {
            // If original was MOV, just move the value to target
            if (target_reg != X86_REG_EAX) {
                uint8_t mov_target_eax[] = {0x89, 0xC0};
                mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
                buffer_append(b, mov_target_eax, 2);
            }
        }
    }
}

strategy_t lea_arithmetic_add_strategy = {
    .name = "LEA-based Arithmetic Addition",
    .can_handle = can_handle_lea_arithmetic_add,
    .get_size = get_size_lea_arithmetic_add,
    .generate = generate_lea_arithmetic_add,
    .priority = 78  // High priority
};

strategy_t lea_multiplication_addition_strategy = {
    .name = "LEA Multiplication and Addition",
    .can_handle = can_handle_lea_multiplication_addition,
    .get_size = get_size_lea_multiplication_addition,
    .generate = generate_lea_multiplication_addition,
    .priority = 75  // Slightly lower priority
};

void register_lea_arithmetic_calculation_strategies() {
    register_strategy(&lea_arithmetic_add_strategy);
    register_strategy(&lea_multiplication_addition_strategy);
}