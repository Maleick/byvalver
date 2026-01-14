/*
 * PUSH-POP Immediate Loading Strategy
 *
 * PROBLEM: Direct MOV with immediate values containing null bytes.
 * 
 * SOLUTION: Use PUSH/POP sequences to load immediate values without 
 * directly encoding them in MOV instructions that might contain nulls.
 *
 * FREQUENCY: Common in shellcode when MOV immediate has null bytes
 * PRIORITY: 77 (High - good for immediate loading without nulls)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00123456 (contains null in first byte)
 *   Strategy: PUSH 0x00123456 (if encoded null-free) or use arithmetic to build it, then POP EAX
 */

#include "push_pop_immediate_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with immediate values containing null bytes
 */
int can_handle_push_pop_immediate(cs_insn *insn) {
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
            // Check if the original MOV instruction encoding contains nulls too
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
 * Size calculation for PUSH-POP immediate loading
 * PUSH imm32 (if null-free) + POP reg = 5 + 1 = 6 bytes
 * If PUSH contains nulls, we need to build the value differently first
 */
size_t get_size_push_pop_immediate(cs_insn *insn) {
    (void)insn; // Unused parameter
    // PUSH constructed_value (multiple bytes) + POP target_reg (1 byte)
    return 10; // Conservative estimate
}

/*
 * Generate PUSH-POP immediate loading
 *
 * For MOV reg, imm32 that contains null bytes in immediate:
 *   If we can PUSH the immediate safely: PUSH imm32; POP reg
 *   Otherwise: Build the value using null-free components first
 */
void generate_push_pop_immediate(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    uint32_t value_to_load = (uint32_t)src_op->imm;

    // Check if we can PUSH the value directly without creating nulls
    if (is_bad_byte_free(value_to_load)) {
        // We can PUSH the value directly
        // PUSH value_to_load
        generate_push_imm32(b, value_to_load);
        
        // POP target_reg
        uint8_t pop_reg = 0x58 + get_reg_index(target_reg);
        buffer_write_byte(b, pop_reg);
    } else {
        // The immediate value contains nulls, so we can't PUSH it directly
        // We need to build it in a null-free way first, then use PUSH-POP approach
        
        // One approach: XOR target register to zero, then add parts that don't contain nulls
        // Another approach: Use an alternative register to build the value
        
        // Save the target register if it's EAX, otherwise save EAX
        int eax_saved = 0;
        if (target_reg == X86_REG_EAX) {
            uint8_t push_target = 0x50 + get_reg_index(target_reg);
            buffer_write_byte(b, push_target);
        } else {
            uint8_t push_eax[] = {0x50};
            buffer_append(b, push_eax, 1);
            eax_saved = 1;
        }
        
        // Build the value in EAX using null-free operations
        generate_mov_eax_imm(b, value_to_load);
        
        // Now we can PUSH EAX and POP the target register
        // PUSH EAX 
        uint8_t push_eax_op = 0x50;
        buffer_write_byte(b, push_eax_op);
        
        // POP target_reg
        uint8_t pop_target = 0x58 + get_reg_index(target_reg);
        buffer_write_byte(b, pop_target);
        
        // Restore EAX if we saved it
        if (eax_saved) {
            uint8_t pop_eax[] = {0x58};
            buffer_append(b, pop_eax, 1);
        }
    }
}

/*
 * Alternative PUSH-POP strategy for arithmetic construction
 */
int can_handle_push_pop_arithmetic_construction(cs_insn *insn) {
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

    // Check if the instruction contains null bytes
    for (size_t j = 0; j < insn->size; j++) {
        if (insn->bytes[j] == 0x00) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_push_pop_arithmetic_construction(cs_insn *insn) {
    (void)insn; // Unused parameter
    return 12; // More complex construction
}

void generate_push_pop_arithmetic_construction(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg target_reg = dst_op->reg;
    uint32_t value_to_load = (uint32_t)src_op->imm;

    // Save target register
    if (target_reg != X86_REG_ESP) {  // Avoid issues with stack pointer
        uint8_t push_target = 0x50 + get_reg_index(target_reg);
        buffer_write_byte(b, push_target);
    }
    
    // Build value in another register (EAX) using null-free operations
    generate_mov_eax_imm(b, value_to_load);
    
    // PUSH the constructed value from EAX
    uint8_t push_eax_code = 0x50;
    buffer_write_byte(b, push_eax_code);
    
    // POP to target register
    uint8_t pop_target = 0x58 + get_reg_index(target_reg);
    buffer_write_byte(b, pop_target);
    
    // In this approach, we're essentially using the stack as temporary storage
    // to transfer a null-free constructed value to the target register
}

strategy_t push_pop_immediate_strategy = {
    .name = "PUSH-POP Immediate Loading",
    .can_handle = can_handle_push_pop_immediate,
    .get_size = get_size_push_pop_immediate,
    .generate = generate_push_pop_immediate,
    .priority = 77  // High priority
};

strategy_t push_pop_arithmetic_construction_strategy = {
    .name = "PUSH-POP Arithmetic Construction",
    .can_handle = can_handle_push_pop_arithmetic_construction,
    .get_size = get_size_push_pop_arithmetic_construction,
    .generate = generate_push_pop_arithmetic_construction,
    .priority = 74  // Slightly lower priority
};

void register_push_pop_immediate_strategies() {
    register_strategy(&push_pop_immediate_strategy);
    register_strategy(&push_pop_arithmetic_construction_strategy);
}