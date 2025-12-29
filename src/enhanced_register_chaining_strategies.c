#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Enhanced strategy for register chaining with immediate values
int can_handle_register_chaining_immediate_enhanced(cs_insn *insn) {
    // This strategy should handle cases where we need to chain operations
    // through multiple registers to avoid null bytes
    if (insn->detail->x86.op_count < 2) {
        return 0;
    }

    // Look for instructions that manipulate registers with immediates that have nulls
    // This is mainly for MOV register, immediate where the immediate contains nulls
    if (insn->id == X86_INS_MOV && 
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_byte_free(imm)) {
            // This should be handled by mov_strategies, but we'll add as backup
            // with higher priority to catch cases that other strategies miss
            return 1;
        }
    }
    
    // Look for arithmetic operations with immediate that contains nulls
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_byte_free(imm)) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_register_chaining_immediate_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH/POP + MOV + operation = ~10-20 bytes depending on complexity
    return 20;
}

void generate_register_chaining_immediate_enhanced(struct buffer *b, cs_insn *insn) {
    // Handle register chaining approach for MOV with null-containing immediate
    if (insn->id == X86_INS_MOV &&
        insn->detail->x86.operands[0].type == X86_OP_REG &&
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Choose temporary register different from destination
        x86_reg temp_reg = X86_REG_EAX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_ECX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_EDX;
        
        // Save temp register
        uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
        buffer_append(b, push_temp, 1);
        
        // MOV temp_reg, imm (null-free construction)
        generate_mov_eax_imm(b, imm);
        
        // MOV temp_reg, EAX (after eax was loaded with imm)
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        // MOV dest_reg, temp_reg
        uint8_t mov_dest_temp[] = {0x89, 0xC0};
        mov_dest_temp[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(dest_reg);
        buffer_append(b, mov_dest_temp, 2);
        
        // Restore temp register
        uint8_t pop_temp[] = {0x58 + get_reg_index(temp_reg)};
        buffer_append(b, pop_temp, 1);
    }
    // Handle arithmetic operations with null-containing immediates
    else if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
              insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
              insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
             insn->detail->x86.operands[0].type == X86_OP_REG && 
             insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Choose temporary register different from destination
        x86_reg temp_reg = X86_REG_EAX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_ECX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_EDX;
        
        // Save temp register
        uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
        buffer_append(b, push_temp, 1);
        
        // MOV temp_reg, imm (null-free construction)
        generate_mov_eax_imm(b, imm);
        
        // MOV temp_reg, EAX (after eax was loaded with imm)
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        // Apply operation: operation dest_reg, temp_reg
        uint8_t op_code = 0;
        switch(insn->id) {
            case X86_INS_ADD: op_code = 0x01; break;  // ADD r32, r32
            case X86_INS_SUB: op_code = 0x29; break;  // SUB r32, r32
            case X86_INS_AND: op_code = 0x21; break;  // AND r32, r32
            case X86_INS_OR:  op_code = 0x09; break;  // OR r32, r32
            case X86_INS_XOR: op_code = 0x31; break;  // XOR r32, r32
            case X86_INS_CMP: op_code = 0x39; break;  // CMP r32, r32
            default: op_code = 0x01; break;  // Default to ADD
        }
        
        uint8_t op_code_bytes[] = {op_code, 0xC0};
        op_code_bytes[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(dest_reg);
        buffer_append(b, op_code_bytes, 2);
        
        // Restore temp register
        uint8_t pop_temp[] = {0x58 + get_reg_index(temp_reg)};
        buffer_append(b, pop_temp, 1);
    }
    else {
        // Fallback to original instruction
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t register_chaining_immediate_enhanced_strategy = {
    .name = "register_chaining_immediate_enhanced",
    .can_handle = can_handle_register_chaining_immediate_enhanced,
    .get_size = get_size_register_chaining_immediate_enhanced,
    .generate = generate_register_chaining_immediate_enhanced,
    .priority = 70  // Medium-high priority
};

// Cross register operation strategy
int can_handle_cross_register_operation_enhanced(cs_insn *insn) {
    // This strategy handles cross-register operations where we need to move through
    // multiple registers to avoid null bytes
    if (insn->detail->x86.op_count < 2) {
        return 0;
    }

    // Look for operations that can be split across multiple steps
    if ((insn->id == X86_INS_MOV) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_byte_free(imm)) {
            // If this immediate can't be handled by standard strategies, try cross-register
            return 1;
        }
    }

    return 0;
}

size_t get_size_cross_register_operation_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH + multiple MOVs + POP = ~15-25 bytes
    return 25;
}

void generate_cross_register_operation_enhanced(struct buffer *b, cs_insn *insn) {
    if (insn->id == X86_INS_MOV &&
        insn->detail->x86.operands[0].type == X86_OP_REG &&
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Use a multi-register approach: EAX -> ECX -> dest_reg
        // Save EAX and ECX
        uint8_t push_eax[] = {0x50};
        uint8_t push_ecx[] = {0x51};
        buffer_append(b, push_eax, 1);
        buffer_append(b, push_ecx, 1);
        
        // MOV EAX, imm (null-free construction)
        generate_mov_eax_imm(b, imm);
        
        // MOV ECX, EAX
        uint8_t mov_ecx_eax[] = {0x89, 0xC1};
        buffer_append(b, mov_ecx_eax, 2);
        
        // MOV dest_reg, ECX
        uint8_t mov_dest_ecx[] = {0x89, 0xC8};
        mov_dest_ecx[1] = 0xC8 + get_reg_index(dest_reg);
        buffer_append(b, mov_dest_ecx, 2);
        
        // Restore ECX and EAX
        uint8_t pop_ecx[] = {0x59};
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_ecx, 1);
        buffer_append(b, pop_eax, 1);
    }
    else {
        // Fallback
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t cross_register_operation_enhanced_strategy = {
    .name = "cross_register_operation_enhanced",
    .can_handle = can_handle_cross_register_operation_enhanced,
    .get_size = get_size_cross_register_operation_enhanced,
    .generate = generate_cross_register_operation_enhanced,
    .priority = 68  // Medium priority
};

void register_enhanced_register_chaining_strategies() {
    register_strategy(&register_chaining_immediate_enhanced_strategy);
    register_strategy(&cross_register_operation_enhanced_strategy);
}