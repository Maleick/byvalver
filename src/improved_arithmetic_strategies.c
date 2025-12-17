#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy: Arithmetic NEG (ADD/SUB with NEG immediate) - handles ADD/SUB reg, imm with nulls
int can_handle_arithmetic_neg_proper(cs_insn *insn) {
    if ((insn->id != X86_INS_ADD && insn->id != X86_INS_SUB &&
         insn->id != X86_INS_AND && insn->id != X86_INS_OR &&
         insn->id != X86_INS_XOR && insn->id != X86_INS_CMP) ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate contains null bytes but negated value doesn't
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm)) {
        uint32_t negated_val = ~imm + 1; // Two's complement negation
        if (is_bad_char_free(negated_val)) {
            return 1;
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_neg_proper(__attribute__((unused)) cs_insn *insn) {
    // PUSH temp; MOV temp, negated_val; NEG temp; appropriate_op reg, temp; POP temp
    return 15; // Conservative estimate
}

void generate_arithmetic_neg_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // Calculate negated value
    uint32_t negated_val = ~imm + 1;
    
    // Choose a temporary register different from target reg
    x86_reg temp_reg = X86_REG_ECX;
    if (temp_reg == reg) temp_reg = X86_REG_EDX;
    if (temp_reg == reg) temp_reg = X86_REG_EBX;
    
    // PUSH temp_reg
    uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
    buffer_append(b, push_temp, 1);
    
    // MOV temp_reg, negated_val (null-free)
    if (temp_reg == X86_REG_EAX) {
        generate_mov_eax_imm(b, negated_val);
    } else {
        // Use EAX to move to temp_reg
        uint8_t push_eax[] = {0x50};
        buffer_append(b, push_eax, 1);
        
        generate_mov_eax_imm(b, negated_val);
        
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_eax, 1);
    }
    
    // NEG temp_reg to get original value back
    uint8_t neg_temp[] = {0xF7, 0xD8};
    neg_temp[1] = 0xD8 + get_reg_index(temp_reg);
    buffer_append(b, neg_temp, 2);
    
    // Perform the original operation with temp_reg
    uint8_t op_code = 0;
    switch(insn->id) {
        case X86_INS_ADD: op_code = 0x01; break;  // ADD r32, r32
        case X86_INS_SUB: op_code = 0x29; break;  // SUB r32, r32
        case X86_INS_AND: op_code = 0x21; break;  // AND r32, r32
        case X86_INS_OR:  op_code = 0x09; break;  // OR r32, r32
        case X86_INS_XOR: op_code = 0x31; break;  // XOR r32, r32
        case X86_INS_CMP: op_code = 0x39; break;  // CMP r32, r32
        default: op_code = 0x01; break;
    }
    
    uint8_t op_code_bytes[] = {op_code, 0xC0};
    op_code_bytes[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(reg);
    buffer_append(b, op_code_bytes, 2);
    
    // POP temp_reg
    uint8_t pop_temp[] = {0x58 + get_reg_index(temp_reg)};
    buffer_append(b, pop_temp, 1);
}

strategy_t arithmetic_neg_proper_strategy = {
    .name = "arithmetic_neg_proper",
    .can_handle = can_handle_arithmetic_neg_proper,
    .get_size = get_size_arithmetic_neg_proper,
    .generate = generate_arithmetic_neg_proper,
    .priority = 80
};

// Strategy: Arithmetic XOR - handles arithmetic operations with XOR-encoding
int can_handle_arithmetic_xor_proper(cs_insn *insn) {
    if ((insn->id != X86_INS_ADD && insn->id != X86_INS_SUB &&
         insn->id != X86_INS_AND && insn->id != X86_INS_OR &&
         insn->id != X86_INS_XOR && insn->id != X86_INS_CMP) ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate contains null bytes but can be XOR-encoded with null-free values
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm)) {
        // Try to find a null-free XOR key
        uint32_t xor_keys[] = {
            0x01010101, 0x11111111, 0x22222222, 0x33333333,
            0x44444444, 0x55555555, 0x66666666, 0x77777777,
            0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB,
            0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF
        };

        for (size_t i = 0; i < sizeof(xor_keys)/sizeof(xor_keys[0]); i++) {
            uint32_t encoded = imm ^ xor_keys[i];
            if (is_bad_char_free(encoded) && is_bad_char_free(xor_keys[i])) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_xor_proper(__attribute__((unused)) cs_insn *insn) {
    // PUSH temp; MOV temp, encoded_val; XOR temp, key; appropriate_op reg, temp; POP temp
    return 20; // Conservative estimate
}

void generate_arithmetic_xor_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // Try to find a null-free XOR key
    uint32_t xor_keys[] = {
        0x01010101, 0x11111111, 0x22222222, 0x33333333,
        0x44444444, 0x55555555, 0x66666666, 0x77777777,
        0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB,
        0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF
    };
    
    uint32_t encoded_val = 0, used_key = 0;
    int found = 0;
    
    for (size_t i = 0; i < sizeof(xor_keys)/sizeof(xor_keys[0]); i++) {
        uint32_t encoded = imm ^ xor_keys[i];
        if (is_bad_char_free(encoded) && is_bad_char_free(xor_keys[i])) {
            encoded_val = encoded;
            used_key = xor_keys[i];
            found = 1;
            break;
        }
    }
    
    if (!found) {
        // Fallback to original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }
    
    // Choose a temporary register different from target reg
    x86_reg temp_reg = X86_REG_ECX;
    if (temp_reg == reg) temp_reg = X86_REG_EDX;
    if (temp_reg == reg) temp_reg = X86_REG_EBX;
    
    // PUSH temp_reg
    uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
    buffer_append(b, push_temp, 1);
    
    // MOV temp_reg, encoded_val (null-free)
    if (temp_reg == X86_REG_EAX) {
        generate_mov_eax_imm(b, encoded_val);
    } else {
        uint8_t push_eax[] = {0x50};
        buffer_append(b, push_eax, 1);
        
        generate_mov_eax_imm(b, encoded_val);
        
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_eax, 1);
    }
    
    // XOR temp_reg, used_key
    if (temp_reg == X86_REG_EAX) {
        uint8_t xor_eax_key[] = {0x35, 0, 0, 0, 0};  // XOR EAX, imm32
        memcpy(xor_eax_key + 1, &used_key, 4);
        buffer_append(b, xor_eax_key, 5);
    } else {
        uint8_t xor_reg_key[] = {0x81, 0xF0, 0, 0, 0, 0};  // XOR reg, imm32
        xor_reg_key[1] = 0xF0 + get_reg_index(temp_reg);
        memcpy(xor_reg_key + 2, &used_key, 4);
        buffer_append(b, xor_reg_key, 6);
    }
    
    // Perform the original operation with temp_reg
    uint8_t op_code = 0;
    switch(insn->id) {
        case X86_INS_ADD: op_code = 0x01; break;  // ADD r32, r32
        case X86_INS_SUB: op_code = 0x29; break;  // SUB r32, r32
        case X86_INS_AND: op_code = 0x21; break;  // AND r32, r32
        case X86_INS_OR:  op_code = 0x09; break;  // OR r32, r32
        case X86_INS_XOR: op_code = 0x31; break;  // XOR r32, r32 (this would cancel out the XOR we just did!)
        case X86_INS_CMP: op_code = 0x39; break;  // CMP r32, r32
        default: op_code = 0x01; break;
    }
    
    // Special handling for XOR - since XOR temp, key followed by XOR reg, temp
    // would effectively do XOR reg, key, we need to handle differently
    if (insn->id == X86_INS_XOR) {
        // We already have the correct value in temp_reg, do XOR reg, temp
        uint8_t op_code_bytes[] = {op_code, 0xC0};
        op_code_bytes[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(reg);
        buffer_append(b, op_code_bytes, 2);
    } else {
        uint8_t op_code_bytes[] = {op_code, 0xC0};
        op_code_bytes[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(reg);
        buffer_append(b, op_code_bytes, 2);
    }
    
    // POP temp_reg
    uint8_t pop_temp[] = {0x58 + get_reg_index(temp_reg)};
    buffer_append(b, pop_temp, 1);
}

strategy_t arithmetic_xor_proper_strategy = {
    .name = "arithmetic_xor_proper",
    .can_handle = can_handle_arithmetic_xor_proper,
    .get_size = get_size_arithmetic_xor_proper,
    .generate = generate_arithmetic_xor_proper,
    .priority = 78
};

// Strategy: Arithmetic ADD/SUB - handles arithmetic operations with ADD/SUB immediate encoding
int can_handle_arithmetic_addsub_proper(cs_insn *insn) {
    if ((insn->id != X86_INS_ADD && insn->id != X86_INS_SUB &&
         insn->id != X86_INS_AND && insn->id != X86_INS_OR &&
         insn->id != X86_INS_XOR && insn->id != X86_INS_CMP) ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate contains null bytes but can be encoded as ADD reg, val1; ADD reg, val2 where val1+val2=imm
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm)) {
        // Try finding two values that sum to imm and are both null-free
        for (uint32_t val1 = 1; val1 < imm && val1 < 0x7FFFFFFF; val1 += 0x01010101) { // Use step to try different values
            uint32_t val2 = imm - val1;
            if (is_bad_char_free(val1) && is_bad_char_free(val2)) {
                return 1;
            }
        }
    }
    
    return 0;
}

size_t get_size_arithmetic_addsub_proper(__attribute__((unused)) cs_insn *insn) {
    // PUSH temp; MOV temp, val1; ADD temp, val2; appropriate_op reg, temp; POP temp
    return 20; // Conservative estimate
}

void generate_arithmetic_addsub_proper(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg reg = insn->detail->x86.operands[0].reg;
    
    // Find two values that sum to imm and are both null-free
    uint32_t val1 = 0, val2 = 0;
    int found = 0;
    
    // Simple approach: try common values
    uint32_t test_values[] = {0x01010101, 0x02020202, 0x04040404, 0x08080808, 
                              0x10101010, 0x20202020, 0x40404040};
    int num_values = sizeof(test_values)/sizeof(test_values[0]);
    
    for (int i = 0; i < num_values; i++) {
        if (test_values[i] < imm) {
            uint32_t temp_val2 = imm - test_values[i];
            if (is_bad_char_free(test_values[i]) && is_bad_char_free(temp_val2)) {
                val1 = test_values[i];
                val2 = temp_val2;
                found = 1;
                break;
            }
        }
    }
    
    // If that doesn't work, try other approaches
    if (!found) {
        for (uint32_t v1 = 0x01010101; v1 < imm && v1 < 0x7FFFFFFF && !found; v1 += 0x01010101) {
            uint32_t v2 = imm - v1;
            if (is_bad_char_free(v1) && is_bad_char_free(v2)) {
                val1 = v1;
                val2 = v2;
                found = 1;
                break;
            }
        }
    }
    
    if (!found) {
        // Fallback to original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }
    
    // Choose a temporary register different from target reg
    x86_reg temp_reg = X86_REG_ECX;
    if (temp_reg == reg) temp_reg = X86_REG_EDX;
    if (temp_reg == reg) temp_reg = X86_REG_EBX;
    
    // PUSH temp_reg
    uint8_t push_temp[] = {0x50 + get_reg_index(temp_reg)};
    buffer_append(b, push_temp, 1);
    
    // MOV temp_reg, val1 (null-free)
    if (temp_reg == X86_REG_EAX) {
        generate_mov_eax_imm(b, val1);
    } else {
        uint8_t push_eax[] = {0x50};
        buffer_append(b, push_eax, 1);
        
        generate_mov_eax_imm(b, val1);
        
        uint8_t mov_temp_eax[] = {0x89, 0xC0};
        mov_temp_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(temp_reg);
        buffer_append(b, mov_temp_eax, 2);
        
        uint8_t pop_eax[] = {0x58};
        buffer_append(b, pop_eax, 1);
    }
    
    // ADD temp_reg, val2 (null-free)
    uint8_t add_code[] = {0x81, 0x00, 0, 0, 0, 0};
    add_code[1] = 0xC0 + get_reg_index(temp_reg);
    memcpy(add_code + 2, &val2, 4);
    buffer_append(b, add_code, 6);
    
    // Perform the original operation with temp_reg
    uint8_t op_code = 0;
    switch(insn->id) {
        case X86_INS_ADD: op_code = 0x01; break;  // ADD r32, r32
        case X86_INS_SUB: op_code = 0x29; break;  // SUB r32, r32
        case X86_INS_AND: op_code = 0x21; break;  // AND r32, r32
        case X86_INS_OR:  op_code = 0x09; break;  // OR r32, r32
        case X86_INS_XOR: op_code = 0x31; break;  // XOR r32, r32
        case X86_INS_CMP: op_code = 0x39; break;  // CMP r32, r32
        default: op_code = 0x01; break;
    }
    
    uint8_t op_code_bytes[] = {op_code, 0xC0};
    op_code_bytes[1] = 0xC0 + (get_reg_index(temp_reg) << 3) + get_reg_index(reg);
    buffer_append(b, op_code_bytes, 2);
    
    // POP temp_reg
    uint8_t pop_temp[] = {0x58 + get_reg_index(temp_reg)};
    buffer_append(b, pop_temp, 1);
}

strategy_t arithmetic_addsub_proper_strategy = {
    .name = "arithmetic_addsub_proper",
    .can_handle = can_handle_arithmetic_addsub_proper,
    .get_size = get_size_arithmetic_addsub_proper,
    .generate = generate_arithmetic_addsub_proper,
    .priority = 76
};

void register_improved_arithmetic_strategies() {
    register_strategy(&arithmetic_neg_proper_strategy);
    register_strategy(&arithmetic_xor_proper_strategy);
    register_strategy(&arithmetic_addsub_proper_strategy);
}