#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 8: Arithmetic/Bitwise Constant Generation
// Generate constants (e.g., 0x10, 0x300) through register manipulation, 
// increment/decrement, or bitwise operations instead of direct immediate 
// values that might contain nulls.

int can_handle_arithmetic_const_generation(cs_insn *insn) {
    // Check for MOV instructions with immediate values that contain nulls
    if (insn->id == X86_INS_MOV) {
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *src_op = &insn->detail->x86.operands[1];
            if (src_op->type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)src_op->imm;
                
                // Check if immediate contains null bytes
                for (int i = 0; i < 4; i++) {
                    if (((imm >> (i * 8)) & 0xFF) == 0) {
                        return 1;
                    }
                }
            }
        }
    }
    
    // Also check for other instructions with immediate values that have nulls
    if (insn->detail->x86.op_count >= 1) {
        cs_x86_op *op = NULL;
        
        // Check first operand for immediate
        if (insn->detail->x86.operands[0].type == X86_OP_IMM) {
            op = &insn->detail->x86.operands[0];
        }
        // Check second operand for immediate (for 2-operand instructions)
        else if (insn->detail->x86.op_count > 1 && insn->detail->x86.operands[1].type == X86_OP_IMM) {
            op = &insn->detail->x86.operands[1];
        }
        
        if (op) {
            uint32_t imm = (uint32_t)op->imm;
            
            for (int i = 0; i < 4; i++) {
                if (((imm >> (i * 8)) & 0xFF) == 0) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_const_generation(__attribute__((unused)) cs_insn *insn) {
    // This would involve multiple arithmetic operations to generate the constant
    // Could be several instructions: MOV, ADD, SUB, XOR, etc.
    return 12; // Conservative estimate
}

void generate_arithmetic_const_generation(struct buffer *b, cs_insn *insn) {
    // For now, fall back to the existing null-free MOV generation
    // which already uses various arithmetic approaches
    
    if (insn->id == X86_INS_MOV && 
        insn->detail->x86.op_count == 2 && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg target_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Use the existing generate_mov_eax_imm which handles null bytes via 
        // various techniques (arithmetic, XOR, etc.)
        generate_mov_eax_imm(b, imm);
        
        // If target reg is not EAX, move from EAX to target
        if (target_reg != X86_REG_EAX) {
            uint8_t mov_target_eax[] = {0x89, 0x00};
            mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
            buffer_append(b, mov_target_eax, 2);
        }
    } else {
        // For other instruction types, use a general approach
        buffer_append(b, insn->bytes, insn->size);
    }
}

// Alternative strategy: Use arithmetic operations to build values
int can_handle_arithmetic_build_value(cs_insn *insn) {
    // Look for immediate values that could be constructed through arithmetic
    if (insn->id == X86_INS_MOV) {
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *src_op = &insn->detail->x86.operands[1];
            if (src_op->type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)src_op->imm;
                
                // Check if immediate contains null bytes
                for (int i = 0; i < 4; i++) {
                    if (((imm >> (i * 8)) & 0xFF) == 0) {
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_build_value(__attribute__((unused)) cs_insn *insn) {
    // This would involve: MOV base reg, null-free-value; OP reg, adjustment
    return 10; // Estimate for MOV + arithmetic operation
}

void generate_arithmetic_build_value(struct buffer *b, cs_insn *insn) {
    if (insn->id == X86_INS_MOV && 
        insn->detail->x86.op_count == 2 && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg target_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Try to find an arithmetic equivalent: base + adjustment = imm
        uint32_t base, adjustment;
        int operation; // 0 for addition, 1 for subtraction
        
        if (find_arithmetic_equivalent(imm, &base, &adjustment, &operation)) {
            // MOV target_reg, base (using null-free construction)
            generate_mov_eax_imm(b, base);
            
            // Move from EAX to target reg if needed
            if (target_reg != X86_REG_EAX) {
                uint8_t mov_target_eax[] = {0x89, 0x00};
                mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
                buffer_append(b, mov_target_eax, 2);
            }
            
            // Perform the arithmetic operation
            if (operation == 0) { // Addition
                // ADD target_reg, adjustment
                if ((int32_t)(int8_t)adjustment == (int32_t)adjustment) {
                    // Use 8-bit signed immediate if possible
                    uint8_t add_reg_imm8[] = {0x83, 0xC0, (uint8_t)adjustment};
                    add_reg_imm8[1] = add_reg_imm8[1] + get_reg_index(target_reg);
                    buffer_append(b, add_reg_imm8, 3);
                } else {
                    // Use 32-bit immediate
                    uint8_t add_reg_imm32[] = {0x83, 0xC0, 0x00, 0x00, 0x00, 0x00};
                    add_reg_imm32[0] = 0x81; // 32-bit version
                    add_reg_imm32[1] = add_reg_imm32[1] + get_reg_index(target_reg);
                    memcpy(add_reg_imm32 + 2, &adjustment, 4);
                    buffer_append(b, add_reg_imm32, 6);
                }
            } else { // Subtraction
                // SUB target_reg, adjustment
                if ((int32_t)(int8_t)adjustment == (int32_t)adjustment) {
                    // Use 8-bit signed immediate if possible
                    uint8_t sub_reg_imm8[] = {0x83, 0xE8, (uint8_t)adjustment};
                    sub_reg_imm8[1] = sub_reg_imm8[1] + get_reg_index(target_reg);
                    buffer_append(b, sub_reg_imm8, 3);
                } else {
                    // Use 32-bit immediate
                    uint8_t sub_reg_imm32[] = {0x83, 0xE8, 0x00, 0x00, 0x00, 0x00};
                    sub_reg_imm32[0] = 0x81; // 32-bit version
                    sub_reg_imm32[1] = sub_reg_imm32[1] + get_reg_index(target_reg);
                    memcpy(sub_reg_imm32 + 2, &adjustment, 4);
                    buffer_append(b, sub_reg_imm32, 6);
                }
            }
        } else {
            // If we can't find a good arithmetic equivalent, fall back
            generate_mov_eax_imm(b, imm);
            
            // If target reg is not EAX, move from EAX to target
            if (target_reg != X86_REG_EAX) {
                uint8_t mov_target_eax[] = {0x89, 0x00};
                mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
                buffer_append(b, mov_target_eax, 2);
            }
        }
    } else {
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t arithmetic_const_generation_strategy = {
    .name = "arithmetic_const_generation",
    .can_handle = can_handle_arithmetic_const_generation,
    .get_size = get_size_arithmetic_const_generation,
    .generate = generate_arithmetic_const_generation,
    .priority = 70  // Medium-high priority
};

strategy_t arithmetic_build_value_strategy = {
    .name = "arithmetic_build_value",
    .can_handle = can_handle_arithmetic_build_value,
    .get_size = get_size_arithmetic_build_value,
    .generate = generate_arithmetic_build_value,
    .priority = 75  // Higher priority for more specific arithmetic approach
};

// Register the arithmetic/bitwise constant generation strategies
void register_arithmetic_const_generation_strategies() {
    register_strategy(&arithmetic_const_generation_strategy);
    register_strategy(&arithmetic_build_value_strategy);
}