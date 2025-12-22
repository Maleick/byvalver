/*
 * Partial Register Optimization Strategy for Bad Character Elimination
 *
 * PROBLEM: Instructions using partial registers (AL, AH, BL, BH, etc.)
 * may result in encodings that contain bad characters, particularly in
 * ModR/M bytes or as immediate values.
 *
 * SOLUTION: Replace partial register operations with equivalent full
 * register operations or alternative encodings that avoid bad characters.
 */

#include "partial_register_optimization_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Transform partial register operations that contain bad characters
 *
 * Original: MOV AL, 0x00 (contains null byte)
 * Transform: MOV EAX, 0x00000000 or XOR EAX, EAX (then use AL)
 */
int can_handle_partial_register_optimization(cs_insn *insn) {
    if (!insn) {
        return 0;
    }
    
    // Check if this is a MOV instruction involving partial registers
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];
        
        // Check if either operand is a partial register (8-bit: AL, AH, BL, BH, etc.)
        if ((dst_op->type == X86_OP_REG && 
             (dst_op->reg >= X86_REG_AL && dst_op->reg <= X86_REG_BH)) ||
            (src_op->type == X86_OP_REG && 
             (src_op->reg >= X86_REG_AL && src_op->reg <= X86_REG_BH))) {
            return 1;
        }
        
        // Also check for MOV with immediate that might contain bad chars
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // Check if destination is a partial register
            if (dst_op->reg >= X86_REG_AL && dst_op->reg <= X86_REG_BH) {
                return 1;
            }
        }
    }
    
    // Check for other instructions with partial registers
    if (insn->detail->x86.op_count > 0) {
        for (int i = 0; i < insn->detail->x86.op_count; i++) {
            if (insn->detail->x86.operands[i].type == X86_OP_REG &&
                insn->detail->x86.operands[i].reg >= X86_REG_AL && 
                insn->detail->x86.operands[i].reg <= X86_REG_BH) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_partial_register_optimization(__attribute__((unused)) cs_insn *insn) {
    // Varies depending on transformation, but typically 2-6 bytes
    return 6;  // Conservative estimate
}

void generate_partial_register_optimization(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }
    
    // Check if this is a MOV instruction with partial register and immediate
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];
        
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // If destination is a partial register and immediate contains bad chars
            if (dst_op->reg >= X86_REG_AL && dst_op->reg <= X86_REG_BH) {
                uint8_t imm_val = (uint8_t)src_op->imm;
                
                // Check if immediate contains bad character (simplified null check)
                if (imm_val == 0x00) {
                    // Transform MOV AL, 0x00 to XOR EAX, EAX (if AL is part of EAX)
                    x86_reg full_reg = X86_REG_EAX;  // Default to EAX
                    
                    // Map partial register to its full register
                    switch(dst_op->reg) {
                        case X86_REG_AL:
                        case X86_REG_AH:
                            full_reg = X86_REG_EAX;
                            break;
                        case X86_REG_BL:
                        case X86_REG_BH:
                            full_reg = X86_REG_EBX;
                            break;
                        case X86_REG_CL:
                        case X86_REG_CH:
                            full_reg = X86_REG_ECX;
                            break;
                        case X86_REG_DL:
                        case X86_REG_DH:
                            full_reg = X86_REG_EDX;
                            break;
                        default:
                            full_reg = X86_REG_EAX;  // Fallback
                            break;
                    }
                    
                    // Use XOR reg, reg to zero the full register
                    uint8_t reg_idx = get_reg_index(full_reg);
                    buffer_write_byte(b, 0x31);  // XOR reg, reg
                    buffer_write_byte(b, 0xC0 | (reg_idx << 3) | reg_idx);  // MOD/RM byte
                    return;
                }
                else {
                    // For non-zero immediate values, we can use MOV with full register
                    // if the immediate doesn't contain bad chars in its 32-bit form
                    x86_reg full_reg = X86_REG_EAX;  // Default to EAX
                    
                    // Map partial register to its full register
                    switch(dst_op->reg) {
                        case X86_REG_AL:
                        case X86_REG_AH:
                            full_reg = X86_REG_EAX;
                            break;
                        case X86_REG_BL:
                        case X86_REG_BH:
                            full_reg = X86_REG_EBX;
                            break;
                        case X86_REG_CL:
                        case X86_REG_CH:
                            full_reg = X86_REG_ECX;
                            break;
                        case X86_REG_DL:
                        case X86_REG_DH:
                            full_reg = X86_REG_EDX;
                            break;
                        default:
                            full_reg = X86_REG_EAX;  // Fallback
                            break;
                    }
                    
                    // Use MOV reg32, imm32
                    uint8_t reg_idx = get_reg_index(full_reg);
                    buffer_write_byte(b, 0xB8 + reg_idx);  // MOV reg32, imm32
                    buffer_write_byte(b, imm_val);  // Immediate value
                    // Fill upper bytes with zeros to maintain same effect
                    buffer_write_byte(b, 0x00);
                    buffer_write_byte(b, 0x00);
                    buffer_write_byte(b, 0x00);
                    return;
                }
            }
        }
    }
    
    // For other partial register operations, try to use equivalent full register ops
    // or alternative encodings
    buffer_append(b, insn->bytes, insn->size);
}

/**
 * Transform operations to avoid partial register dependencies that cause bad chars
 */
int can_handle_partial_register_dependency(cs_insn *insn) {
    if (!insn) {
        return 0;
    }
    
    // Check for instructions that may have bad chars due to partial register usage
    // This is a simplified check - in practice, we'd check the actual encoding
    if (insn->detail->x86.op_count > 0) {
        for (int i = 0; i < insn->detail->x86.op_count; i++) {
            if (insn->detail->x86.operands[i].type == X86_OP_REG &&
                insn->detail->x86.operands[i].reg >= X86_REG_AL && 
                insn->detail->x86.operands[i].reg <= X86_REG_BH) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_partial_register_dependency(__attribute__((unused)) cs_insn *insn) {
    return 5;  // Conservative estimate
}

void generate_partial_register_dependency(struct buffer *b, cs_insn *insn) {
    // For this implementation, we'll use the main function
    generate_partial_register_optimization(b, insn);
}

// Define the strategy structure
strategy_t partial_register_optimization_strategy = {
    .name = "Partial Register Optimization",
    .can_handle = can_handle_partial_register_dependency,
    .get_size = get_size_partial_register_dependency,
    .generate = generate_partial_register_dependency,
    .priority = 89  // As specified in documentation
};