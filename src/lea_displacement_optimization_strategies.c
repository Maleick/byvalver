/*
 * LEA Displacement Optimization Strategies
 *
 * This strategy module handles LEA (Load Effective Address) instructions
 * that contain null bytes in displacement values. LEA is commonly used
 * with displacement addressing modes that can contain null bytes.
 */

#include "strategy.h"
#include "utils.h"
#include "lea_displacement_optimization_strategies.h"
#include <stdio.h>
#include <string.h>

/*
 * Detection for LEA instructions with null-byte displacements
 * LEA reg, [base + disp32] where disp32 contains null bytes
 */
int can_handle_lea_displacement_nulls(cs_insn *insn) {
    if (insn->id != X86_INS_LEA) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    if (dst->type != X86_OP_REG || src->type != X86_OP_MEM) {
        return 0;
    }

    // Check if the memory operand has displacement that contains null bytes
    if (src->mem.disp != 0) {
        uint64_t disp = src->mem.disp;
        for (int i = 0; i < 8; i++) {
            if (((disp >> (i * 8)) & 0xFF) == 0x00) {
                return 1;
            }
        }
    }
    
    // Also check if entire instruction contains null bytes (in encoding)
    for (int i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1;
        }
    }

    return 0;
}

/*
 * Detection for LEA with problematic addressing that creates null encoding
 */
int can_handle_lea_problematic_encoding(cs_insn *insn) {
    if (insn->id != X86_INS_LEA) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *src = &insn->detail->x86.operands[1];
    if (src->type != X86_OP_MEM) {
        return 0;
    }

    // Check for addressing modes that create problematic encodings
    // For example, when using EBP/R13 as base without displacement
    if ((src->mem.base == X86_REG_EBP || src->mem.base == X86_REG_R13) && 
        src->mem.disp == 0) {
        // This requires a displacement byte (0x00), which is a null
        return 1;
    }

    return 0;
}

size_t get_size_lea_displacement_nulls(__attribute__((unused)) cs_insn *insn) {
    // Alternative LEA patterns might require additional instructions
    return 15;
}

size_t get_size_lea_problematic_encoding(__attribute__((unused)) cs_insn *insn) {
    // Complex LEA rewrites might need several instructions
    return 18;
}

/*
 * Generate LEA with null-byte displacement replacement
 */
void generate_lea_displacement_nulls(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];
    
    x86_reg dst_reg = dst->reg;
    x86_reg base_reg = src->mem.base;
    x86_reg index_reg = src->mem.index;
    uint32_t scale = src->mem.scale;
    uint64_t disp = src->mem.disp;
    
    // If we have a simple LEA reg, [base + disp] where disp contains nulls:
    if (index_reg == X86_REG_INVALID && scale == 1) {
        // Alternative approach: use register arithmetic to avoid null displacement
        // MOV dst_reg, base_reg; ADD dst_reg, displacement (constructed without nulls)
        
        // First, MOV dst_reg, base_reg
        if (base_reg != dst_reg) {
            uint8_t mov_reg[] = {0x89, 0xC0 + (get_reg_index(base_reg) << 3) + get_reg_index(dst_reg)};
            buffer_append(b, mov_reg, 2);
        }
        
        // ADD dst_reg, displacement using utilities that handle nulls
        generate_op_reg_imm(b, &(cs_insn){
            .id = X86_INS_ADD,
            .detail = &(cs_detail){
                .x86 = {
                    .op_count = 2,
                    .operands = {{.type = X86_OP_REG, .reg = dst_reg}, {.type = X86_OP_IMM, .imm = disp}}
                }
            }
        });
    } 
    else if (index_reg != X86_REG_INVALID) {
        // For complex addressing [base + index*scale + disp], we need a different approach
        x86_reg temp_reg = X86_REG_EAX;
        if (dst_reg == X86_REG_EAX) {
            temp_reg = X86_REG_ECX;
            if (dst_reg == X86_REG_ECX) {
                temp_reg = X86_REG_EDX;
            }
        }
        
        // MOV dst_reg, base_reg
        if (base_reg != dst_reg) {
            generate_mov_reg_imm(b, &(cs_insn){
                .id = X86_INS_MOV,
                .detail = &(cs_detail){
                    .x86 = {
                        .op_count = 2,
                        .operands = {{.type = X86_OP_REG, .reg = dst_reg}, {.type = X86_OP_REG, .reg = base_reg}}
                    }
                }
            });
        }
        
        // MOV temp_reg, index_reg
        generate_mov_reg_imm(b, &(cs_insn){
            .id = X86_INS_MOV,
            .detail = &(cs_detail){
                .x86 = {
                    .op_count = 2,
                    .operands = {{.type = X86_OP_REG, .reg = temp_reg}, {.type = X86_OP_REG, .reg = index_reg}}
                }
            }
        });
        
        // If scale > 1, multiply temp_reg by scale
        if (scale > 1) {
            generate_op_reg_imm(b, &(cs_insn){
                .id = X86_INS_IMUL,
                .detail = &(cs_detail){
                    .x86 = {
                        .op_count = 2,
                        .operands = {{.type = X86_OP_REG, .reg = temp_reg}, {.type = X86_OP_IMM, .imm = scale}}
                    }
                }
            });
        }
        
        // ADD dst_reg, temp_reg (add the scaled index)
        generate_op_reg_imm(b, &(cs_insn){
            .id = X86_INS_ADD,
            .detail = &(cs_detail){
                .x86 = {
                    .op_count = 2,
                    .operands = {{.type = X86_OP_REG, .reg = dst_reg}, {.type = X86_OP_REG, .reg = temp_reg}}
                }
            }
        });
        
        // ADD dst_reg, displacement (constructed without nulls)
        generate_op_reg_imm(b, &(cs_insn){
            .id = X86_INS_ADD,
            .detail = &(cs_detail){
                .x86 = {
                    .op_count = 2,
                    .operands = {{.type = X86_OP_REG, .reg = dst_reg}, {.type = X86_OP_IMM, .imm = disp}}
                }
            }
        });
    }
    else {
        // Fallback: append original if we can't handle it effectively
        buffer_append(b, insn->bytes, insn->size);
    }
}

/*
 * Generate alternative for LEA with problematic encoding
 */
void generate_lea_problematic_encoding(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];
    
    x86_reg dst_reg = dst->reg;
    x86_reg base_reg = src->mem.base;
    uint64_t disp = src->mem.disp;
    
    // Handle addressing mode that requires displacement byte (like EBP/R13 + 0)
    if ((base_reg == X86_REG_EBP || base_reg == X86_REG_R13) && disp == 0) {
        // Instead of LEA dst, [EBP] which requires 00 displacement byte,
        // use: MOV dst_reg, base_reg
        generate_mov_reg_imm(b, &(cs_insn){
            .id = X86_INS_MOV,
            .detail = &(cs_detail){
                .x86 = {
                    .op_count = 2,
                    .operands = {{.type = X86_OP_REG, .reg = dst_reg}, {.type = X86_OP_REG, .reg = base_reg}}
                }
            }
        });
    } else {
        // Use the same approach as the general displacement handler
        generate_lea_displacement_nulls(b, insn);
    }
}

/*
 * Strategy definitions
 */
strategy_t lea_displacement_nulls_strategy = {
    .name = "lea_displacement_nulls",
    .can_handle = can_handle_lea_displacement_nulls,
    .get_size = get_size_lea_displacement_nulls,
    .generate = generate_lea_displacement_nulls,
    .priority = 82  // Medium-high priority for LEA operations
};

strategy_t lea_problematic_encoding_strategy = {
    .name = "lea_problematic_encoding",
    .can_handle = can_handle_lea_problematic_encoding,
    .get_size = get_size_lea_problematic_encoding,
    .generate = generate_lea_problematic_encoding,
    .priority = 81  // Slightly lower than displacement_nulls
};

/*
 * Register function
 */
void register_lea_displacement_optimization_strategies() {
    register_strategy(&lea_displacement_nulls_strategy);
    register_strategy(&lea_problematic_encoding_strategy);
}