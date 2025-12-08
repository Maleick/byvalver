/*
 * Arithmetic Flag Preservation Strategy
 *
 * PROBLEM: When replacing instructions that modify flags, we need to preserve the
 * flag state for subsequent conditional operations, but using operations that avoid
 * null bytes in immediate operands.
 *
 * SOLUTION: Use arithmetic operations that preserve flags while avoiding null bytes
 * in immediate operands, or use flag-preserving sequences of operations.
 *
 * FREQUENCY: Common when replacing conditional operations or flag-setting instructions
 * PRIORITY: 87 (High - important for maintaining correct program flow)
 *
 * Example transformations:
 *   Original: ADD EAX, 0x00001000 (contains null bytes in immediate)
 *   Strategy: Use sequences of operations that preserve flags but avoid nulls
 */

#include "arithmetic_flag_preservation_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for arithmetic operations with immediate values containing null bytes
 */
int can_handle_arithmetic_flag_preservation(cs_insn *insn) {
    // Check for arithmetic instructions with immediate values that contain nulls
    if (!(insn->id == X86_INS_ADD || insn->id == X86_INS_SUB ||
          insn->id == X86_INS_AND || insn->id == X86_INS_OR ||
          insn->id == X86_INS_XOR || insn->id == X86_INS_CMP)) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *src_op = NULL;

    // For most arithmetic ops, the immediate is the second operand
    if (insn->detail->x86.operands[1].type == X86_OP_IMM) {
        src_op = &insn->detail->x86.operands[1];
    }

    if (!src_op) {
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
 * Size calculation for arithmetic flag preservation
 */
size_t get_size_arithmetic_flag_preservation(cs_insn *insn) {
    (void)insn; // Unused parameter

    // May require multiple instructions to maintain flags while avoiding nulls
    return 15; // Conservative estimate
}

/*
 * Generate arithmetic flag preservation
 *
 * For arithmetic operations with immediate containing nulls:
 *   Use flag-preserving sequences that avoid null immediate values
 */
void generate_arithmetic_flag_preservation(struct buffer *b, cs_insn *insn) {
    // For now, this will be a more complex implementation that maintains flag state
    // while using null-free immediate values
    
    // First, identify the operation type
    x86_insn op_id = insn->id;
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    // Save flags or use alternate approach to maintain flag state
    // PUSHFD to save flags (if needed)
    uint8_t pushfd[] = {0x9C};
    buffer_append(b, pushfd, 1);
    
    // We'll implement a generic approach that avoids the immediate with nulls
    // Use a temporary register to load the immediate value without nulls
    x86_reg temp_reg = X86_REG_ECX;
    if (dst_op->reg == X86_REG_ECX) {
        temp_reg = X86_REG_EDX;  // Choose a different temp register if ECX is in use
        if (dst_op->reg == X86_REG_EDX) {
            temp_reg = X86_REG_EBX;  // Choose another if both ECX and EDX are in use
        }
    }
    
    // MOV temp_reg, immediate_value (constructed without nulls)
    generate_mov_eax_imm(b, (uint32_t)src_op->imm);
    
    // MOV temp_reg, EAX to move the value to our chosen temp register
    uint8_t mov_temp_eax[] = {0x89, 0xC0};
    mov_temp_eax[1] = (get_reg_index(temp_reg) << 3) | get_reg_index(X86_REG_EAX);
    buffer_append(b, mov_temp_eax, 2);
    
    // Now perform the arithmetic operation using the temp register instead of immediate
    uint8_t arithmetic_opcode = 0x00;
    int recognized_op = 1; // Flag to check if the operation was recognized

    switch(op_id) {
        case X86_INS_ADD:  // ADD
            arithmetic_opcode = 0x01;
            break;
        case X86_INS_SUB:  // SUB
            arithmetic_opcode = 0x29;
            break;
        case X86_INS_AND:  // AND
            arithmetic_opcode = 0x21;
            break;
        case X86_INS_OR:   // OR
            arithmetic_opcode = 0x09;
            break;
        case X86_INS_XOR:  // XOR
            arithmetic_opcode = 0x31;
            break;
        case X86_INS_CMP:  // CMP (same as SUB but doesn't store result)
            arithmetic_opcode = 0x39;
            break;
        default:
            recognized_op = 0; // Operation not recognized
            break;
    }

    if (!recognized_op) {
        // If we don't recognize the op, fall back to original
        buffer_append(b, insn->bytes, insn->size);
        uint8_t popfd[] = {0x9D};
        buffer_append(b, popfd, 1);
        return;
    }
    
    // Format: arithmetic_opcode [dst_reg], temp_reg
    uint8_t arithmetic_instr[] = {arithmetic_opcode, 0x00};
    // Determine the ModR/M byte for [destination], source
    // For reg, reg format: ModR/M = 11 (reg mode) + (dst_reg << 3) + src_reg
    arithmetic_instr[1] = (0xC0) | (get_reg_index(dst_op->reg) << 3) | get_reg_index(temp_reg);
    buffer_append(b, arithmetic_instr, 2);
    
    // Restore flags if needed (though this approach should maintain the same flag state)
    uint8_t popfd[] = {0x9D};
    buffer_append(b, popfd, 1);
}

/*
 * Alternative approach: Use arithmetic with flag-preserving decomposition
 */
int can_handle_flag_preserving_decomposition(cs_insn *insn) {
    return can_handle_arithmetic_flag_preservation(insn);
}

size_t get_size_flag_preserving_decomposition(cs_insn *insn) {
    (void)insn;
    return 18;  // Might be larger due to decomposition
}

void generate_flag_preserving_decomposition(struct buffer *b, cs_insn *insn) {
    // This is a simpler alternative that decomposes arithmetic operations
    // into sequences that avoid null bytes while preserving flags
    
    x86_insn op_id = insn->id;
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    // Save destination register if needed
    if (dst_op->reg != X86_REG_EAX) {
        uint8_t push_target[] = {0x50};
        push_target[0] |= get_reg_index(dst_op->reg);  // Encode target register in PUSH
        buffer_append(b, push_target, 1);
    }
    
    // Load the immediate value into EAX using null-free construction
    generate_mov_eax_imm(b, (uint32_t)src_op->imm);
    
    // Restore the original destination value to another register if needed
    if (dst_op->reg != X86_REG_EAX) {
        uint8_t pop_target[] = {0x58};
        pop_target[0] |= get_reg_index(dst_op->reg); // Encode target register in POP
        buffer_append(b, pop_target, 1);
        
        // Now perform the arithmetic operation between the target reg and EAX
        uint8_t arithmetic_opcode = 0x00;
        switch(op_id) {
            case X86_INS_ADD:  arithmetic_opcode = 0x01; break;  // ADD dst_reg, EAX
            case X86_INS_SUB:  arithmetic_opcode = 0x29; break;  // SUB EAX, dst_reg -> need to swap operands
            case X86_INS_AND:  arithmetic_opcode = 0x21; break;  // AND dst_reg, EAX
            case X86_INS_OR:   arithmetic_opcode = 0x09; break;  // OR dst_reg, EAX
            case X86_INS_XOR:  arithmetic_opcode = 0x31; break;  // XOR dst_reg, EAX
            case X86_INS_CMP:  arithmetic_opcode = 0x39; break;  // CMP dst_reg, EAX
            default:
                // For unhandled instructions, use a default (should not happen in practice)
                // This silences the warning while maintaining functionality for the supported operations
                break;
        }
        
        if (op_id == X86_INS_SUB) {
            // For SUB, we need different opcode to do dst - eax: 0x29
            // SUB dst_reg, EAX (dst = dst - eax)
            uint8_t sub_instr[] = {0x29, 0x00};
            sub_instr[1] = (0xC0) | (get_reg_index(dst_op->reg) << 3) | get_reg_index(X86_REG_EAX);
            buffer_append(b, sub_instr, 2);
        } else {
            // For other ops: op dst_reg, EAX
            uint8_t arithmetic_instr[] = {arithmetic_opcode, 0x00};
            arithmetic_instr[1] = (0xC0) | (get_reg_index(dst_op->reg) << 3) | get_reg_index(X86_REG_EAX);
            buffer_append(b, arithmetic_instr, 2);
        }
    }
}

strategy_t arithmetic_flag_preservation_strategy = {
    .name = "Arithmetic Flag Preservation",
    .can_handle = can_handle_arithmetic_flag_preservation,
    .get_size = get_size_arithmetic_flag_preservation,
    .generate = generate_arithmetic_flag_preservation,
    .priority = 87  // High priority
};

strategy_t flag_preserving_decomposition_strategy = {
    .name = "Flag Preserving Decomposition",
    .can_handle = can_handle_flag_preserving_decomposition,
    .get_size = get_size_flag_preserving_decomposition,
    .generate = generate_flag_preserving_decomposition,
    .priority = 85  // High priority
};

void register_arithmetic_flag_preservation_strategies() {
    register_strategy(&arithmetic_flag_preservation_strategy);
    register_strategy(&flag_preserving_decomposition_strategy);
}