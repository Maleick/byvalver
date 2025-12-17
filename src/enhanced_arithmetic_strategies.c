#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Enhanced arithmetic negation strategy
int can_handle_arithmetic_neg_enhanced(cs_insn *insn) {
    // Look for arithmetic operations with immediate that has null bytes
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_char_free(imm)) {
            uint32_t negated_val;
            if (find_neg_equivalent(imm, &negated_val) && is_bad_char_free(negated_val)) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_neg_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH + MOV + NEG + OP + POP = ~15-20 bytes
    return 20;
}

void generate_arithmetic_neg_enhanced(struct buffer *b, cs_insn *insn) {
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Find negated value that is null-free
        uint32_t negated_val;
        if (!find_neg_equivalent(imm, &negated_val)) {
            // Fallback if no negated value found
            buffer_append(b, insn->bytes, insn->size);
            return;
        }
        
        // Choose temporary register different from destination
        x86_reg temp_reg = X86_REG_EAX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_ECX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_EDX;
        
        // Save temp register
        uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
        buffer_append(b, push_temp, 1);
        
        // MOV temp_reg, negated_val (null-free)
        generate_mov_eax_imm(b, negated_val);
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        // NEG temp_reg (to get original value back)
        uint8_t neg_temp[] = {0xF7, 0xD8};
        neg_temp[1] = 0xD8 + get_reg_index(temp_reg);
        buffer_append(b, neg_temp, 2);
        
        // Apply operation: op dest_reg, temp_reg
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
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t arithmetic_neg_enhanced_strategy = {
    .name = "arithmetic_neg_enhanced",
    .can_handle = can_handle_arithmetic_neg_enhanced,
    .get_size = get_size_arithmetic_neg_enhanced,
    .generate = generate_arithmetic_neg_enhanced,
    .priority = 75  // High priority for negation-based strategies
};

// Enhanced arithmetic XOR strategy
int can_handle_arithmetic_xor_enhanced(cs_insn *insn) {
    // Look for arithmetic operations with immediate that has null bytes
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_char_free(imm)) {
            uint32_t xor_key;
            if (find_xor_key(imm, &xor_key)) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_xor_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH + MOV + XOR + OP + POP = ~20-25 bytes
    return 25;
}

void generate_arithmetic_xor_enhanced(struct buffer *b, cs_insn *insn) {
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Find XOR key that is null-free
        uint32_t xor_key;
        if (!find_xor_key(imm, &xor_key)) {
            // Fallback if no XOR key found
            buffer_append(b, insn->bytes, insn->size);
            return;
        }
        
        // XOR-encoded value: imm ^ xor_key
        uint32_t encoded_val = imm ^ xor_key;
        
        // Choose temporary register different from destination
        x86_reg temp_reg = X86_REG_EAX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_ECX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_EDX;
        
        // Save temp register
        uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
        buffer_append(b, push_temp, 1);
        
        // MOV temp_reg, encoded_val (null-free)
        generate_mov_eax_imm(b, encoded_val);
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        // XOR temp_reg, xor_key
        if (temp_reg == X86_REG_EAX) {
            // XOR EAX, imm32 (uses 0x35 opcode)
            uint8_t xor_eax_key[] = {0x35, 0, 0, 0, 0};
            memcpy(xor_eax_key + 1, &xor_key, 4);
            buffer_append(b, xor_eax_key, 5);
        } else {
            // XOR reg, imm32 (uses 0x83/0x81 with /6)
            uint8_t xor_reg_key[] = {0x81, 0xF0, 0, 0, 0, 0};
            xor_reg_key[1] = 0xF0 + get_reg_index(temp_reg);  // /6 for XOR
            memcpy(xor_reg_key + 2, &xor_key, 4);
            buffer_append(b, xor_reg_key, 6);
        }
        
        // Apply operation: op dest_reg, temp_reg
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
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t arithmetic_xor_enhanced_strategy = {
    .name = "arithmetic_xor_enhanced",
    .can_handle = can_handle_arithmetic_xor_enhanced,
    .get_size = get_size_arithmetic_xor_enhanced,
    .generate = generate_arithmetic_xor_enhanced,
    .priority = 72  // High priority for XOR-based strategies
};

// Enhanced arithmetic ADD/SUB strategy
int can_handle_arithmetic_addsub_enhanced(cs_insn *insn) {
    // Look for arithmetic operations with immediate that has null bytes
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        if (!is_bad_char_free(imm)) {
            uint32_t val1, val2;
            int is_add;
            if (find_addsub_key(imm, &val1, &val2, &is_add)) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_addsub_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH + MOV + ADD/SUB + OP + POP = ~20-25 bytes
    return 25;
}

void generate_arithmetic_addsub_enhanced(struct buffer *b, cs_insn *insn) {
    if ((insn->id == X86_INS_ADD || insn->id == X86_INS_SUB || 
         insn->id == X86_INS_AND || insn->id == X86_INS_OR || 
         insn->id == X86_INS_XOR || insn->id == X86_INS_CMP) &&
        insn->detail->x86.operands[0].type == X86_OP_REG && 
        insn->detail->x86.operands[1].type == X86_OP_IMM) {
        
        x86_reg dest_reg = insn->detail->x86.operands[0].reg;
        uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
        
        // Find ADD/SUB values that are null-free
        uint32_t val1, val2;
        int is_add;
        if (!find_addsub_key(imm, &val1, &val2, &is_add)) {
            // Fallback if no ADD/SUB values found
            buffer_append(b, insn->bytes, insn->size);
            return;
        }
        
        // Choose temporary register different from destination
        x86_reg temp_reg = X86_REG_EAX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_ECX;
        if (temp_reg == dest_reg) temp_reg = X86_REG_EDX;
        
        // Save temp register
        uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
        buffer_append(b, push_temp, 1);
        
        // MOV temp_reg, val1 (null-free)
        generate_mov_eax_imm(b, val1);
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        // ADD/SUB temp_reg, val2
        if (is_add) {
            // ADD temp_reg, val2
            uint8_t add_code[] = {0x81, 0xC0, 0, 0, 0, 0};
            add_code[1] = 0xC0 + get_reg_index(temp_reg);
            memcpy(add_code + 2, &val2, 4);
            buffer_append(b, add_code, 6);
        } else {
            // SUB temp_reg, val2
            uint8_t sub_code[] = {0x81, 0xE8, 0, 0, 0, 0};
            sub_code[1] = 0xE8 + get_reg_index(temp_reg);
            memcpy(sub_code + 2, &val2, 4);
            buffer_append(b, sub_code, 6);
        }
        
        // Apply operation: op dest_reg, temp_reg
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
        buffer_append(b, insn->bytes, insn->size);
    }
}

strategy_t arithmetic_addsub_enhanced_strategy = {
    .name = "arithmetic_addsub_enhanced",
    .can_handle = can_handle_arithmetic_addsub_enhanced,
    .get_size = get_size_arithmetic_addsub_enhanced,
    .generate = generate_arithmetic_addsub_enhanced,
    .priority = 70  // Medium-high priority for ADD/SUB-based strategies
};

void register_enhanced_arithmetic_strategies() {
    register_strategy(&arithmetic_neg_enhanced_strategy);
    register_strategy(&arithmetic_xor_enhanced_strategy);
    register_strategy(&arithmetic_addsub_enhanced_strategy);
}