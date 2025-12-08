/*
 * Bit Manipulation Constant Strategy
 *
 * PROBLEM: Creating constants that contain null bytes in immediate values
 * can introduce null bytes in the instruction encoding.
 *
 * SOLUTION: Generate constants through sequences of bit manipulation operations
 * like shifts, rotates, XOR, AND, OR that don't require immediate values with nulls.
 *
 * FREQUENCY: Useful for creating complex constants without null bytes
 * PRIORITY: 84 (High - important for complex constant generation)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00123456 (contains null in opcode)
 *   Strategy: Use sequences of bit operations to construct the value without null immediates
 */

#include "bit_manipulation_constant_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with immediate constants containing null bytes
 */
int can_handle_bit_manipulation_constant(cs_insn *insn) {
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
 * Size calculation for bit manipulation constant generation
 */
size_t get_size_bit_manipulation_constant(cs_insn *insn) {
    (void)insn; // Unused parameter

    // Requires multiple bit manipulation operations
    return 20; // Conservative estimate for complex bit operations
}

/*
 * Generate bit manipulation constant
 *
 * For MOV reg, const_immediate that contains nulls:
 *   Use bit manipulation sequences to construct the value without direct null encoding
 */
void generate_bit_manipulation_constant(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t const_value = (uint32_t)src_op->imm;
    
    // We'll build the constant using bit operations like shifts and ORs
    // For example, if const_value = 0x00123456, we could:
    // XOR EAX, EAX (clear EAX)
    // MOV AL, 0x56 (set lowest byte)
    // SHL EAX, 8
    // OR AL, 0x34
    // SHL EAX, 8
    // OR AL, 0x12
    // SHL EAX, 8
    // (No need for highest byte since it's 0x00)
    
    // Start with clearing the target register
    uint8_t xor_reg_reg[] = {0x31, 0x00};
    xor_reg_reg[1] = (get_reg_index(target_reg) << 3) | get_reg_index(target_reg);
    buffer_append(b, xor_reg_reg, 2);
    
    // Get individual bytes of the constant (in reverse order to build from lowest to highest)
    uint8_t bytes[4];
    bytes[0] = (const_value >> 0) & 0xFF;   // Lowest byte
    bytes[1] = (const_value >> 8) & 0xFF;
    bytes[2] = (const_value >> 16) & 0xFF;
    bytes[3] = (const_value >> 24) & 0xFF;  // Highest byte
    
    // Build the value byte by byte
    for (int i = 3; i >= 0; i--) {
        if (i < 3) { // For all but the highest byte, shift first
            // SHL target_reg, 8
            uint8_t shl_reg_8[] = {0xC1, 0xE0, 0x08};
            shl_reg_8[1] = 0xE0 | get_reg_index(target_reg);  // Encode register in SHL
            buffer_append(b, shl_reg_8, 3);
        }
        
        // If this byte is not zero, set it
        if (bytes[i] != 0) {
            // MOV the byte to the AL register (or appropriate low byte register)
            // For EAX, AL is accessed via the same register with different opcodes
            uint8_t mov_al_byte[] = {0x00, 0x00};
            if (target_reg == X86_REG_EAX) {
                // MOV AL, bytes[i] (B0 + byte)
                mov_al_byte[0] = 0xB0;
                mov_al_byte[1] = bytes[i];
                buffer_append(b, mov_al_byte, 2);
            } else if (target_reg == X86_REG_EBX) {
                mov_al_byte[0] = 0xB3;  // MOV BL, bytes[i]
                mov_al_byte[1] = bytes[i];
                buffer_append(b, mov_al_byte, 2);
            } else if (target_reg == X86_REG_ECX) {
                mov_al_byte[0] = 0xB1;  // MOV CL, bytes[i]
                mov_al_byte[1] = bytes[i];
                buffer_append(b, mov_al_byte, 2);
            } else if (target_reg == X86_REG_EDX) {
                mov_al_byte[0] = 0xB2;  // MOV DL, bytes[i]
                mov_al_byte[1] = bytes[i];
                buffer_append(b, mov_al_byte, 2);
            } else {
                // For other registers, we'll use a more general approach
                // MOV target_low_byte, bytes[i]
                // This uses the fact that we can access low bytes of registers in x86
                uint8_t reg_idx = get_reg_index(target_reg);
                mov_al_byte[0] = 0xB0 + (reg_idx & 0x7);  // MOV reg_low_byte, imm8
                mov_al_byte[1] = bytes[i];
                buffer_append(b, mov_al_byte, 2);
            }
        }
    }
}

/*
 * Alternative approach: Use rotation and XOR operations
 */
int can_handle_rotation_xor_constant(cs_insn *insn) {
    return can_handle_bit_manipulation_constant(insn);
}

size_t get_size_rotation_xor_constant(cs_insn *insn) {
    (void)insn;
    return 22;  // Might require more operations for rotation/XOR approach
}

void generate_rotation_xor_constant(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t const_value = (uint32_t)src_op->imm;
    
    // Another approach: use rotations to build the value
    // Start with a known value and rotate/xor to get desired value
    // But this can be complex, so let's use a simpler approach
    
    // Use a temporary approach similar to the main function but with more operations
    // XOR EAX, EAX (clear)
    uint8_t xor_reg_reg[] = {0x31, 0x00};
    xor_reg_reg[1] = (get_reg_index(target_reg) << 3) | get_reg_index(target_reg);
    buffer_append(b, xor_reg_reg, 2);
    
    // Break down the constant into non-null components and build it up
    // For example, 0x00123456 = 0x00120000 + 0x00003456
    // We'll add these components using multiple operations
    uint32_t high_part = (const_value & 0xFF000000);  // Highest byte
    uint32_t mid_part = (const_value & 0x00FF0000);   // Second byte
    uint32_t low_part = (const_value & 0x0000FFFF);   // Lower two bytes
    
    // Build the value step by step using null-free operations
    if (high_part) {
        // Handle high byte if it's not zero
        // This would require more complex handling, but for now we'll use MOV with null-free construction
        generate_mov_eax_imm(b, high_part);
        if (target_reg != X86_REG_EAX) {
            uint8_t mov_target_eax[] = {0x89, 0xC0};
            mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
            buffer_append(b, mov_target_eax, 2);
        }
    }
    
    if (mid_part) {
        // Add the middle part
        generate_mov_eax_imm(b, mid_part);
        // ADD target_reg, EAX
        uint8_t add_target_eax[] = {0x01, 0xC0};
        add_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
        buffer_append(b, add_target_eax, 2);
    }
    
    if (low_part) {
        // Add the low part
        generate_mov_eax_imm(b, low_part);
        // ADD target_reg, EAX
        uint8_t add_target_eax2[] = {0x01, 0xC0};
        add_target_eax2[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
        buffer_append(b, add_target_eax2, 2);
    }
}

strategy_t bit_manipulation_constant_strategy = {
    .name = "Bit Manipulation Constant",
    .can_handle = can_handle_bit_manipulation_constant,
    .get_size = get_size_bit_manipulation_constant,
    .generate = generate_bit_manipulation_constant,
    .priority = 84  // High priority
};

strategy_t rotation_xor_constant_strategy = {
    .name = "Rotation XOR Constant",
    .can_handle = can_handle_rotation_xor_constant,
    .get_size = get_size_rotation_xor_constant,
    .generate = generate_rotation_xor_constant,
    .priority = 82  // High priority
};

void register_bit_manipulation_constant_strategies() {
    register_strategy(&bit_manipulation_constant_strategy);
    register_strategy(&rotation_xor_constant_strategy);
}