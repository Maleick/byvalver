/*
 * Stack String Construction Strategy
 *
 * PROBLEM: Creating strings that contain null bytes (or require immediate values with nulls)
 * in shellcode can introduce null bytes that break string-based operations.
 *
 * SOLUTION: Push string parts (in reverse order, without null bytes) onto the stack,
 * then reference them from the stack location. This avoids embedding null bytes directly
 * in the instruction stream.
 *
 * FREQUENCY: Common when shellcode needs to reference string constants like API names
 * PRIORITY: 88 (High - important for Windows string-based API calls)
 *
 * Example transformations:
 *   Original: MOV EAX, "test" (where "test" contains nulls when padded to dword)
 *   Strategy: PUSH "tset" (reversed); MOV EAX, [ESP] to access string
 */

#include "stack_string_construction_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with string immediates that might contain nulls
 */
int can_handle_stack_string_construction(cs_insn *insn) {
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

    // Check if the immediate value contains null bytes (common in string constants)
    uint32_t imm = (uint32_t)src_op->imm;
    
    // Check if the immediate looks like a string (ASCII values with possible null padding)
    // This is a heuristic - if the immediate looks like ASCII characters, it might be a string
    int null_bytes = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t byte_val = (imm >> (i * 8)) & 0xFF;
        if (byte_val == 0) {
            null_bytes++;
        }
        // Check if byte is a printable ASCII character or null terminator
        else if ((byte_val >= 32 && byte_val <= 126) || byte_val == 0) {
            continue; // Likely ASCII character
        } else {
            // Not a typical ASCII character, might not be a string
            return 0;
        }
    }

    // If we have null bytes in what looks like ASCII chars, use this strategy
    if (null_bytes > 0) {
        return 1;
    }

    return 0;
}

/*
 * Size calculation for stack string construction
 *
 * Transformation uses:
 * - PUSH string_dword (1-6 bytes depending on null-free encoding)
 * - MOV target_reg, [ESP] (2 bytes for register addressing)
 * Total: ~3-8 bytes depending on null-free PUSH construction
 */
size_t get_size_stack_string_construction(cs_insn *insn) {
    (void)insn; // Unused parameter

    // PUSH (2-6 bytes for null-free construction) + MOV reg, [ESP] (2 bytes) = ~4-8 bytes
    return 8;
}

/*
 * Generate stack string construction
 *
 * For MOV reg, string_immediate that contains nulls:
 *   PUSH reversed_string_dword (constructed without nulls)
 *   MOV target_reg, [ESP]
 */
void generate_stack_string_construction(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t string_imm = (uint32_t)src_op->imm;
    
    // Save the current ESP to restore it later
    // PUSH ESP
    uint8_t push_esp[] = {0x54};
    buffer_append(b, push_esp, 1);
    
    // Push the string dword to the stack (constructed without nulls)
    generate_push_imm(b, string_imm);
    
    // MOV target_reg, [ESP] - move value from top of stack to target register
    uint8_t mov_reg_esp[] = {0x8B, 0x00};
    mov_reg_esp[1] = (get_reg_index(target_reg) << 3) | 0x04;  // ModR/M byte
    buffer_append(b, mov_reg_esp, 2);
    
    // Restore ESP to what it was before we pushed
    // POP ESP
    uint8_t pop_esp[] = {0x5C};
    buffer_append(b, pop_esp, 1);
}

/*
 * Alternative strategy: Construct longer strings by pushing multiple dwords
 */
int can_handle_stack_multi_string_construction(cs_insn *insn) {
    // For this example, we'll use the same criteria as the single string approach
    return can_handle_stack_string_construction(insn);
}

size_t get_size_stack_multi_string_construction(cs_insn *insn) {
    (void)insn;
    // Might require multiple operations for longer strings
    return 12;
}

void generate_stack_multi_string_construction(struct buffer *b, cs_insn *insn) {
    // Same as single string construction for now
    generate_stack_string_construction(b, insn);
}

strategy_t stack_string_construction_strategy = {
    .name = "Stack String Construction",
    .can_handle = can_handle_stack_string_construction,
    .get_size = get_size_stack_string_construction,
    .generate = generate_stack_string_construction,
    .priority = 88  // High priority
};

strategy_t stack_multi_string_construction_strategy = {
    .name = "Stack Multi-String Construction",
    .can_handle = can_handle_stack_multi_string_construction,
    .get_size = get_size_stack_multi_string_construction,
    .generate = generate_stack_multi_string_construction,
    .priority = 85  // Slightly lower priority
};

void register_stack_string_construction_strategies() {
    register_strategy(&stack_string_construction_strategy);
    register_strategy(&stack_multi_string_construction_strategy);
}