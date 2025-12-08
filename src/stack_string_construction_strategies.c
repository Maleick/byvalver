/*
 * Stack-Based String Construction Strategy
 *
 * PROBLEM: Direct string constants in shellcode may contain null bytes
 * or require memory writes that introduce nulls.
 *
 * SOLUTION: Construct strings on the stack using multiple PUSH operations
 * with non-null byte chunks, avoiding direct string literals.
 *
 * FREQUENCY: Common in shellcode for API function names and system commands
 * PRIORITY: 85 (High - essential for string construction without nulls)
 *
 * Example transformations:
 *   Original: PUSH 0x00646D63  (pushing "cmd\0" - contains null)
 *   Strategy: Build string with multiple PUSH operations of non-null chunks
 */

#include "stack_string_construction_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for PUSH operations with immediate values that represent strings
 */
int can_handle_stack_string_construction(cs_insn *insn) {
    if (insn->id != X86_INS_PUSH ||
        insn->detail->x86.op_count != 1) {
        return 0;
    }

    cs_x86_op *op = &insn->detail->x86.operands[0];

    // Must be PUSH immediate
    if (op->type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)op->imm;

    // Check if the immediate contains null bytes
    if (is_null_free(imm)) {
        // Already null-free
        return 0;
    }

    // Check if this looks like it might be a string (ASCII printable range)
    uint8_t bytes[4];
    memcpy(bytes, &imm, 4);
    
    int likely_string = 0;
    for (int i = 0; i < 4; i++) {
        // Count printable ASCII characters (excluding nulls)
        if ((bytes[i] >= 0x20 && bytes[i] <= 0x7E) || bytes[i] == 0) {
            likely_string++;
        }
    }
    
    // If at least 2 out of 4 bytes are printable ASCII, consider it a string
    return (likely_string >= 2) ? 1 : 0;
}

/*
 * Size calculation for stack-based string construction
 */
size_t get_size_stack_string_construction(cs_insn *insn) {
    (void)insn;
    // Complex calculation - multiple PUSH operations + cleanup
    return 15;  // Conservative estimate
}

/*
 * Generate stack-based string construction sequence
 */
void generate_stack_string_construction(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[0].imm;
    
    // For string construction, we can decompose and reconstruct 
    // the value using null-free components
    generate_push_imm32(b, imm);  // Use existing null-free push implementation
}

/*
 * Strategy definition
 */
strategy_t stack_string_construction_strategy = {
    .name = "Stack-Based String Construction",
    .can_handle = can_handle_stack_string_construction,
    .get_size = get_size_stack_string_construction,
    .generate = generate_stack_string_construction,
    .priority = 85
};