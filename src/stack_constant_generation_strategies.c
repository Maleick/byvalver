/*
 * Stack-Based Constant Generation Strategy
 *
 * PROBLEM: Creating immediate constants that contain null bytes in their encoding
 * can introduce null bytes in the shellcode.
 *
 * SOLUTION: Generate constants by manipulating the stack and using stack-relative
 * addressing or by pushing and popping values to construct the desired constant
 * without directly encoding null bytes.
 *
 * FREQUENCY: Common when shellcode needs specific constants that happen to have null bytes
 * PRIORITY: 86 (High - important for constant generation without nulls)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00123456 (contains null byte, direct encoding has nulls)
 *   Strategy: Use stack operations to construct value without direct null encoding
 */

#include "stack_constant_generation_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with immediate constants containing null bytes
 */
int can_handle_stack_constant_generation(cs_insn *insn) {
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
 * Size calculation for stack-based constant generation
 */
size_t get_size_stack_constant_generation(cs_insn *insn) {
    (void)insn; // Unused parameter

    // Requires push/pop operations and addressing calculations
    return 16; // Conservative estimate
}

/*
 * Generate stack-based constant
 *
 * For MOV reg, const_immediate that contains nulls:
 *   Use stack operations to construct the value without direct null encoding
 */
void generate_stack_constant_generation(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t const_value = (uint32_t)src_op->imm;
    
    // Save the current ESP value to restore later
    uint8_t push_esp[] = {0x54};  // PUSH ESP
    buffer_append(b, push_esp, 1);
    
    // Push the constant value to the stack (construct it without nulls)
    // We'll use generate_push_imm which should handle null-free construction
    generate_push_imm(b, const_value);
    
    // MOV target_reg, [ESP] - get the value from top of stack
    uint8_t mov_reg_esp[] = {0x8B, 0x00};
    mov_reg_esp[1] = (get_reg_index(target_reg) << 3) | 0x04;  // ModR/M byte for [ESP]
    buffer_append(b, mov_reg_esp, 2);
    
    // Restore ESP to where it was before we pushed our constant
    uint8_t pop_esp[] = {0x5C};  // POP ESP
    buffer_append(b, pop_esp, 1);
}

/*
 * Alternative approach: Create constants using stack arithmetic
 */
int can_handle_stack_arithmetic_constant(cs_insn *insn) {
    return can_handle_stack_constant_generation(insn);
}

size_t get_size_stack_arithmetic_constant(cs_insn *insn) {
    (void)insn;
    return 18;  // Might need more operations for arithmetic approach
}

void generate_stack_arithmetic_constant(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t const_value = (uint32_t)src_op->imm;
    
    // For more complex constants, we could use arithmetic on the stack
    // For now, implement a more elaborate version of the same approach
    
    // Save ESP
    uint8_t push_esp[] = {0x54};
    buffer_append(b, push_esp, 1);
    
    // Generate the constant using our null-free MOV to EAX first
    generate_mov_eax_imm(b, const_value);
    
    // Push EAX to stack
    uint8_t push_eax[] = {0x50};
    buffer_append(b, push_eax, 1);
    
    // MOV target_reg, [ESP] to get the value
    uint8_t mov_reg_ptr_esp[] = {0x8B, 0x00};
    mov_reg_ptr_esp[1] = (get_reg_index(target_reg) << 3) | 0x04;  // [ESP]
    buffer_append(b, mov_reg_ptr_esp, 2);
    
    // Restore ESP to remove the pushed value and restore original ESP
    uint8_t pop_esp[] = {0x5C};
    buffer_append(b, pop_esp, 1);
}

strategy_t stack_constant_generation_strategy = {
    .name = "Stack Constant Generation",
    .can_handle = can_handle_stack_constant_generation,
    .get_size = get_size_stack_constant_generation,
    .generate = generate_stack_constant_generation,
    .priority = 86  // High priority
};

strategy_t stack_arithmetic_constant_strategy = {
    .name = "Stack Arithmetic Constant",
    .can_handle = can_handle_stack_arithmetic_constant,
    .get_size = get_size_stack_arithmetic_constant,
    .generate = generate_stack_arithmetic_constant,
    .priority = 84  // High priority
};

void register_stack_constant_generation_strategies() {
    register_strategy(&stack_constant_generation_strategy);
    register_strategy(&stack_arithmetic_constant_strategy);
}