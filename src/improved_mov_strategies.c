#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy: MOV with NEG (x86_NEG - F7 /3) - Handles MOV reg, imm where imm contains nulls via NEG
int can_handle_mov_neg_proper(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate contains null bytes but negated value doesn't
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_byte_free(imm)) {
        uint32_t negated_val = ~imm + 1; // Two's complement negation
        if (is_bad_byte_free(negated_val)) {
            return 1;
        }
    }
    
    return 0;
}

size_t get_size_mov_neg_proper(__attribute__((unused)) cs_insn *insn) {
    return 7; // MOV reg, negated_val (5) + NEG reg (2)
}

void generate_mov_neg_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // Calculate negated value
    uint32_t negated_val = ~imm + 1;
    
    // MOV reg, negated_val (null-free)
    if (reg == X86_REG_EAX) {
        generate_mov_eax_imm(b, negated_val);
    } else {
        // Save EAX
        uint8_t push_eax[] = {0x50};
        buffer_append(b, push_eax, 1);
        
        // Load negated value into EAX
        generate_mov_eax_imm(b, negated_val);
        
        // Move to target register
        uint8_t mov_reg_eax[] = {0x89, 0xC0};
        mov_reg_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(reg);
        buffer_append(b, mov_reg_eax, 2);
        
        // Restore EAX
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_eax, 1);
    }
    
    // NEG reg
    uint8_t neg_code[] = {0xF7, 0xD8};
    neg_code[1] = 0xD8 + get_reg_index(reg);
    buffer_append(b, neg_code, 2);
}

strategy_t mov_neg_proper_strategy = {
    .name = "mov_neg_proper",
    .can_handle = can_handle_mov_neg_proper,
    .get_size = get_size_mov_neg_proper,
    .generate = generate_mov_neg_proper,
    .priority = 75
};

// Strategy: MOV with NOT (x86_NOT - F7 /2) - Handles MOV reg, imm via NOT
int can_handle_mov_not_proper(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate contains null bytes but bitwise NOT value doesn't
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_byte_free(imm)) {
        uint32_t not_val = ~imm; // Bitwise NOT
        if (is_bad_byte_free(not_val)) {
            return 1;
        }
    }
    
    return 0;
}

size_t get_size_mov_not_proper(__attribute__((unused)) cs_insn *insn) {
    return 7; // MOV reg, not_val (5) + NOT reg (2)
}

void generate_mov_not_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // Calculate NOT value
    uint32_t not_val = ~imm;
    
    // MOV reg, not_val (null-free)
    if (reg == X86_REG_EAX) {
        generate_mov_eax_imm(b, not_val);
    } else {
        // Save EAX
        uint8_t push_eax[] = {0x50};
        buffer_append(b, push_eax, 1);
        
        // Load NOT value into EAX
        generate_mov_eax_imm(b, not_val);
        
        // Move to target register
        uint8_t mov_reg_eax[] = {0x89, 0xC0};
        mov_reg_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(reg);
        buffer_append(b, mov_reg_eax, 2);
        
        // Restore EAX
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_eax, 1);
    }
    
    // NOT reg
    uint8_t not_code[] = {0xF7, 0xD0};
    not_code[1] = 0xD0 + get_reg_index(reg);
    buffer_append(b, not_code, 2);
}

strategy_t mov_not_proper_strategy = {
    .name = "mov_not_proper",
    .can_handle = can_handle_mov_not_proper,
    .get_size = get_size_mov_not_proper,
    .generate = generate_mov_not_proper,
    .priority = 74
};

// Strategy: MOV with ADD/SUB decomposition - MOV reg, val via ADD reg, (val-base) if reg initialized to base
int can_handle_mov_addsub_proper(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // This strategy is more complex - we need to know initial register state
    // For now, implement a simpler version that uses XOR to zero first, then ADD
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_byte_free(imm)) {
        // Just check if we can build it with ADD (knowing reg starts as 0 or some known value)
        // For this implementation, if imm is small enough and null-free when we add it to zero reg
        // We'll use XOR reg,reg then ADD/SUB reg,imm approach
        
        // If immediate is positive and small enough, we can do XOR then ADD
        if (imm <= 0x7FFFFFFF) {  // Max positive 32-bit value
            // For now, just return 1 if not handled by other strategies
            return 1;
        }
    }
    
    return 0;
}

size_t get_size_mov_addsub_proper(__attribute__((unused)) cs_insn *insn) {
    return 9; // XOR reg,reg (2) + ADD reg,imm (6) + 1 for safety
}

void generate_mov_addsub_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // XOR reg,reg to zero it
    uint8_t xor_code[] = {0x31, 0x00};
    xor_code[1] = 0xC0 + (get_reg_index(reg) << 3) + get_reg_index(reg);
    buffer_append(b, xor_code, 2);
    
    // ADD reg,imm (where imm is the desired final value)
    // Use 32-bit ADD reg, imm32 format
    uint8_t add_code[] = {0x83, 0x00, 0x00};
    if (imm <= 127) {  // 127 is the max positive value for signed 8-bit
        // Use ADD reg, imm8 with 83 opcode
        add_code[1] = 0xC0 + get_reg_index(reg);
        add_code[2] = (uint8_t)imm;
        buffer_append(b, add_code, 3);
    } else {
        // Use ADD reg, imm32 with 81 opcode (6 bytes total)
        uint8_t add32_code[] = {0x81, 0xC0, 0x00, 0x00, 0x00, 0x00};
        add32_code[1] = 0xC0 + get_reg_index(reg);
        memcpy(add32_code + 2, &imm, 4);
        buffer_append(b, add32_code, 6);
    }
}

strategy_t mov_addsub_proper_strategy = {
    .name = "mov_addsub_proper",
    .can_handle = can_handle_mov_addsub_proper,
    .get_size = get_size_mov_addsub_proper,
    .generate = generate_mov_addsub_proper,
    .priority = 60
};

void register_improved_mov_strategies() {
    register_strategy(&mov_neg_proper_strategy);
    register_strategy(&mov_not_proper_strategy);
    register_strategy(&mov_addsub_proper_strategy);
}