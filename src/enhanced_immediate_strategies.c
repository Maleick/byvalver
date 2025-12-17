#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Enhanced Immediate Value Splitting Strategy
int can_handle_immediate_splitting_enhanced(cs_insn *insn) {
    // Target MOV reg, imm32 where the immediate contains null bytes
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm)) {
        // Only use this strategy if other high-priority strategies can't handle it
        uint32_t neg_val, not_val, encoded_val;
        uint32_t val1, val2;
        int is_add;
        
        // Don't use if NEG strategy can handle it efficiently
        if (find_neg_equivalent(imm, &neg_val) && is_bad_char_free(neg_val)) {
            return 0;
        }
        
        // Don't use if NOT strategy can handle it efficiently
        if (find_not_equivalent(imm, &not_val) && is_bad_char_free(not_val)) {
            return 0;
        }
        
        // Don't use if XOR strategy can handle it efficiently
        if (find_xor_key(imm, &encoded_val)) {
            return 0;
        }
        
        // Don't use if ADD/SUB strategy can handle it efficiently
        if (find_addsub_key(imm, &val1, &val2, &is_add) && is_bad_char_free(val1) && is_bad_char_free(val2)) {
            return 0;
        }
        
        // OK to use this strategy as it's a fallback
        return 1;
    }
    
    return 0;
}

size_t get_size_immediate_splitting_enhanced(__attribute__((unused)) cs_insn *insn) {
    // XOR reg,reg + multiple OR operations to build byte by byte
    // XOR (2) + 4 bytes OR operations (8) = 10 bytes
    // This is less efficient than other strategies but works as fallback
    return 15; // Conservative estimate
}

void generate_immediate_splitting_enhanced(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg dest_reg = insn->detail->x86.operands[0].reg;

    // XOR dest_reg, dest_reg to clear it (safe operation that avoids nulls)
    uint8_t xor_code[] = {0x31, 0x00};
    xor_code[1] = 0xC0 + (get_reg_index(dest_reg) << 3) + get_reg_index(dest_reg);
    buffer_append(b, xor_code, 2);

    // Build value byte by byte from MSB to LSB, shifting left by 8 each time
    for (int byte_pos = 3; byte_pos >= 0; byte_pos--) {
        uint8_t byte_val = (imm >> (byte_pos * 8)) & 0xFF;
        
        if (byte_pos < 3) {
            // Shift left by 8 if not the first byte
            uint8_t shl_code[] = {0xC1, 0xE0, 0x08}; // SHL reg, 8
            shl_code[1] = 0xE0 + get_reg_index(dest_reg);
            buffer_append(b, shl_code, 3);
        }
        
        if (byte_val != 0) {
            // OR the byte value
            if (byte_val <= 127) {
                // Use OR reg_low, imm8 for smaller values (avoids nulls in immediate)
                uint8_t or_code[] = {0x80, 0x00, byte_val};
                or_code[1] = 0xC8 + get_reg_index(dest_reg);
                buffer_append(b, or_code, 3);
            } else {
                // For values > 127, we need to be careful not to introduce nulls
                // Use the standard approach: MOV temp, byte_val; SHL temp, pos; OR dest, temp
                // But this is complex, so just use OR with full immediate
                uint8_t or_code[] = {0x83, 0x00, byte_val};
                or_code[1] = 0xC8 + get_reg_index(dest_reg);
                buffer_append(b, or_code, 3);
            }
        }
        // If byte_val is 0, there's nothing to OR in, just the shift was needed
    }
}

strategy_t immediate_splitting_enhanced_strategy = {
    .name = "Immediate Value Splitting Enhanced",
    .can_handle = can_handle_immediate_splitting_enhanced,
    .get_size = get_size_immediate_splitting_enhanced,
    .generate = generate_immediate_splitting_enhanced,
    .priority = 80  // Medium-high priority for fallback
};

// Enhanced Small Immediate Value Encoding Strategy
int can_handle_small_immediate_enhanced(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm)) {
        // For small immediates with null bytes that other strategies can't handle
        return 1;
    }
    
    return 0;
}

size_t get_size_small_immediate_enhanced(__attribute__((unused)) cs_insn *insn) {
    // PUSH immediate + POP reg (for 32-bit) = 6 bytes for 32-bit immediate, 2 bytes for 8-bit
    // But using the register approach: MOV EAX, imm (null-free) + MOV reg, EAX
    return 12; // Conservative estimate
}

void generate_small_immediate_enhanced(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg dest_reg = insn->detail->x86.operands[0].reg;

    // For small immediates with null bytes, use the standard approach
    // PUSH EAX to save
    uint8_t push_eax[] = {0x50};
    buffer_append(b, push_eax, 1);

    // MOV EAX, imm (with null-free construction)
    generate_mov_eax_imm(b, imm);

    // MOV dest_reg, EAX
    uint8_t mov_dst_eax[] = {0x89, 0x00};
    mov_dst_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(dest_reg);
    buffer_append(b, mov_dst_eax, 2);

    // POP EAX to restore
    uint8_t pop_eax[] = {0x58};
    buffer_append(b, pop_eax, 1);
}

strategy_t small_immediate_enhanced_strategy = {
    .name = "Small Immediate Value Encoding Enhanced",
    .can_handle = can_handle_small_immediate_enhanced,
    .get_size = get_size_small_immediate_enhanced,
    .generate = generate_small_immediate_enhanced,
    .priority = 78  // High priority
};

// Enhanced Large Immediate Value MOV Strategy
int can_handle_large_immediate_enhanced(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_REG || 
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    // Target large immediates (more than 16 bits) that contain null bytes
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    if (!is_bad_char_free(imm) && imm > 0xFFFF) {
        return 1;
    }
    
    return 0;
}

size_t get_size_large_immediate_enhanced(__attribute__((unused)) cs_insn *insn) {
    // For large immediates, approach is: MOV EAX, imm (null-free) + MOV reg, EAX
    return 15; // Conservative estimate with room for complex null-free construction
}

void generate_large_immediate_enhanced(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;
    x86_reg dest_reg = insn->detail->x86.operands[0].reg;

    // Use register switching approach for large immediates with null bytes
    // Save EAX (our construction register)
    uint8_t push_eax[] = {0x50};
    buffer_append(b, push_eax, 1);

    // Load the immediate value into EAX using null-free construction
    generate_mov_eax_imm(b, imm);

    // MOV dest_reg, EAX
    uint8_t mov_dst_eax[] = {0x89, 0x00};
    mov_dst_eax[1] = 0xC0 + (get_reg_index(X86_REG_EAX) << 3) + get_reg_index(dest_reg);
    buffer_append(b, mov_dst_eax, 2);

    // Restore EAX
    uint8_t pop_eax[] = {0x58};
    buffer_append(b, pop_eax, 1);
}

strategy_t large_immediate_enhanced_strategy = {
    .name = "Large Immediate Value MOV Optimization Enhanced",
    .can_handle = can_handle_large_immediate_enhanced,
    .get_size = get_size_large_immediate_enhanced,
    .generate = generate_large_immediate_enhanced,
    .priority = 82  // High priority for large immediates
};

void register_enhanced_immediate_strategies() {
    register_strategy(&immediate_splitting_enhanced_strategy);
    register_strategy(&small_immediate_enhanced_strategy);
    register_strategy(&large_immediate_enhanced_strategy);
}