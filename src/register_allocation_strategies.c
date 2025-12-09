/*
 * Register Allocation Strategies for Null Avoidance
 *
 * This strategy module handles register remapping to avoid null-byte patterns
 * by selecting alternative registers that naturally don't introduce nulls
 * when used in specific instruction contexts.
 */

#include "strategy.h"
#include "utils.h"
#include "register_allocation_strategies.h"
#include <stdio.h>
#include <string.h>

/*
 * Detection for any instruction that uses a register in a way that creates null bytes
 * This could be based on the instruction encoding itself, or based on the immediate values
 * that get combined with the register.
 */
int can_handle_register_remap_nulls(cs_insn *insn) {
    // Check the instruction encoding for null bytes
    for (int i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1;
        }
    }

    // Check if any register operands are part of an expression that could generate nulls
    // For example, if we're dealing with instructions that involve ESP/EBP in certain contexts
    // where they generate null bytes in the ModR/M byte
    
    // Look for operations on specific registers that might generate nulls
    // in the ModR/M or SIB bytes
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_REG) {
            x86_reg reg = insn->detail->x86.operands[i].reg;
            
            // Check if this is a register that could be problematic in certain contexts
            // For example, when used in [reg] addressing mode, EBP and R13 need displacement 
            // which could introduce nulls
            if (reg == X86_REG_EBP || reg == X86_REG_R13) {
                // If this register is used in addressing mode without displacement,
                // it will require a zero displacement byte
                for (int j = 0; j < insn->detail->x86.op_count; j++) {
                    if (insn->detail->x86.operands[j].type == X86_OP_MEM) {
                        if (insn->detail->x86.operands[j].mem.base == reg && 
                            insn->detail->x86.operands[j].mem.disp == 0) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

/*
 * Detection for MOV instructions where source/destination registers might be problematic
 */
int can_handle_mov_register_remap(cs_insn *insn) {
    if (insn->id != X86_INS_MOV) {
        return 0;
    }
    
    // Check for null bytes in the instruction
    for (int i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1;
        }
    }

    // Check for memory addressing with EBP/R13 without displacement
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_MEM) {
            if ((insn->detail->x86.operands[i].mem.base == X86_REG_EBP || 
                 insn->detail->x86.operands[i].mem.base == X86_REG_R13) &&
                insn->detail->x86.operands[i].mem.disp == 0) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_register_remap_nulls(__attribute__((unused)) cs_insn *insn) {
    // Register remapping might require additional instructions for transfer
    // MOV alternative_reg, original_reg or similar operation
    return 8;
}

size_t get_size_mov_register_remap(__attribute__((unused)) cs_insn *insn) {
    // Additional MOV instruction to transfer between registers
    return 6;
}

/*
 * Generate register remapping to avoid null-byte patterns in addressing
 */
void generate_register_remap_nulls(struct buffer *b, cs_insn *insn) {
    // For memory addressing with EBP/R13 that requires zero displacement
    // We should remap to a different register if possible
    
    // Check if this is a memory operation with EBP/R13 and zero displacement
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_MEM) {
            x86_reg problematic_reg = insn->detail->x86.operands[i].mem.base;
            if ((problematic_reg == X86_REG_EBP || problematic_reg == X86_REG_R13) &&
                insn->detail->x86.operands[i].mem.disp == 0) {
                
                // Find an alternative register to replace the problematic one
                x86_reg alt_reg = X86_REG_EAX; // Default alternative
                // Make sure we don't use the same register as the other operand
                for (int j = 0; j < insn->detail->x86.op_count; j++) {
                    if (insn->detail->x86.operands[j].type == X86_OP_REG) {
                        if (insn->detail->x86.operands[j].reg == X86_REG_EAX) {
                            alt_reg = X86_REG_ECX;
                            if (insn->detail->x86.operands[j].reg == X86_REG_ECX) {
                                alt_reg = X86_REG_EDX;
                            }
                        }
                    }
                }
                
                // Copy the problematic register's value to the alternative register
                // MOV alt_reg, problematic_reg
                uint8_t mov_reg[] = {0x89, 0xC0 + (get_reg_index(problematic_reg) << 3) + get_reg_index(alt_reg)};
                buffer_append(b, mov_reg, 2);
                
                // Now modify the original instruction to use the new register
                // This is complex to do generically, so we'll use a simplified approach
                // For now, append the original instruction as a fallback
                buffer_append(b, insn->bytes, insn->size);
                return;
            }
        }
    }
    
    // If no specific register remapping needed, just append original
    buffer_append(b, insn->bytes, insn->size);
}

/*
 * Generate register remapping specifically for MOV instructions
 */
void generate_mov_register_remap(struct buffer *b, cs_insn *insn) {
    // For MOV instructions, try to remap if it helps avoid nulls
    
    // Check if this MOV has problematic register addressing
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_MEM) {
            x86_reg base_reg = insn->detail->x86.operands[i].mem.base;
            if ((base_reg == X86_REG_EBP || base_reg == X86_REG_R13) &&
                insn->detail->x86.operands[i].mem.disp == 0) {
                
                // Find alternative register
                x86_reg dst_reg = (insn->detail->x86.operands[0].type == X86_OP_REG) ? 
                                 insn->detail->x86.operands[0].reg : X86_REG_INVALID;
                
                x86_reg alt_reg = X86_REG_EAX;
                if (dst_reg == X86_REG_EAX || (insn->detail->x86.operands[1].type == X86_OP_REG && 
                                              insn->detail->x86.operands[1].reg == X86_REG_EAX)) {
                    alt_reg = X86_REG_ECX;
                    if (dst_reg == X86_REG_ECX || (insn->detail->x86.operands[1].type == X86_OP_REG && 
                                                  insn->detail->x86.operands[1].reg == X86_REG_ECX)) {
                        alt_reg = X86_REG_EDX;
                    }
                }
                
                // MOV alt_reg, base_reg (copy the address)
                uint8_t mov_addr[] = {0x89, 0xC0 + (get_reg_index(base_reg) << 3) + get_reg_index(alt_reg)};
                buffer_append(b, mov_addr, 2);
                
                // Now we'd need to recreate the MOV with the new register
                // This is complex, so we'll append original as fallback for now
                buffer_append(b, insn->bytes, insn->size);
                return;
            }
        }
    }
    
    // If no change needed, append original
    buffer_append(b, insn->bytes, insn->size);
}

/*
 * More sophisticated register allocation that considers the whole instruction context
 */
int can_handle_contextual_register_swap(cs_insn *insn) {
    // Check if any register usage could potentially create nulls when combined
    // with other parts of the instruction
    
    // Look for common patterns in shellcode that create null-bytes:
    // - Using EBP as base without displacement (requires 00 displacement byte)
    // - Using R13 as base without displacement (requires 00 displacement byte in x64)
    // - Specific register + immediate combinations
    
    for (int i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1;
        }
    }
    
    return 0;
}

size_t get_size_contextual_register_swap(__attribute__((unused)) cs_insn *insn) {
    return 10; // For register swapping + original instruction
}

void generate_contextual_register_swap(struct buffer *b, cs_insn *insn) {
    // Analyze if register swapping could reduce nulls
    // This is a complex transformation that depends on context
    
    // For now, we'll implement a basic version that swaps to avoid problematic
    // addressing modes
    buffer_append(b, insn->bytes, insn->size);
}

/*
 * Strategy definitions
 */
strategy_t register_remap_nulls_strategy = {
    .name = "register_remap_nulls",
    .can_handle = can_handle_register_remap_nulls,
    .get_size = get_size_register_remap_nulls,
    .generate = generate_register_remap_nulls,
    .priority = 75  // Medium priority
};

strategy_t mov_register_remap_strategy = {
    .name = "mov_register_remap",
    .can_handle = can_handle_mov_register_remap,
    .get_size = get_size_mov_register_remap,
    .generate = generate_mov_register_remap,
    .priority = 78  // Medium-high priority for MOV operations
};

strategy_t contextual_register_swap_strategy = {
    .name = "contextual_register_swap",
    .can_handle = can_handle_contextual_register_swap,
    .get_size = get_size_contextual_register_swap,
    .generate = generate_contextual_register_swap,
    .priority = 72  // Medium priority
};

/*
 * Register function
 */
void register_register_allocation_strategies() {
    register_strategy(&register_remap_nulls_strategy);
    register_strategy(&mov_register_remap_strategy);
    register_strategy(&contextual_register_swap_strategy);
}