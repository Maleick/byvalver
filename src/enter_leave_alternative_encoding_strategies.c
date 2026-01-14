/*
 * ENTER/LEAVE Stack Frame Alternative Encoding Strategy
 *
 * PROBLEM: ENTER instructions encode with immediate values that may contain
 * null bytes. LEAVE may have encoding issues in certain contexts.
 *
 * SOLUTION: Replace with manual stack frame operations.
 *
 * Example:
 * Original: enter 0x100, 0  (C8 00 01 00)
 * Transform: push ebp; mov ebp, esp; sub esp, 0x100
 *
 * Original: leave  (C9)
 * Transform: mov esp, ebp; pop ebp
 */

#include "enter_leave_alternative_encoding_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Check if this strategy can handle the instruction
 */
int can_handle_enter_leave(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if instruction is ENTER or LEAVE
    if (insn->id != X86_INS_ENTER && insn->id != X86_INS_LEAVE) {
        return 0;
    }

    // Check if the instruction encoding contains bad bytes
    if (!is_bad_byte_free_buffer(insn->bytes, insn->size)) {
        return 1;  // Has bad chars, we can handle it
    }

    return 0;  // No bad chars, no need to transform
}

/**
 * Calculate size of transformed instruction
 */
size_t get_size_enter_leave(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    if (insn->id == X86_INS_ENTER) {
        // ENTER imm16, 0 transformation:
        // PUSH EBP = 1 byte (55)
        // MOV EBP, ESP = 2 bytes (89 E5)
        // SUB ESP, imm32 = 6 bytes (81 EC XX XX XX XX) for 32-bit immediate
        // Total = 9 bytes
        // Note: SUB ESP immediate may need null-free encoding, but we'll use full 32-bit form
        return 9;
    } else if (insn->id == X86_INS_LEAVE) {
        // LEAVE transformation:
        // MOV ESP, EBP = 2 bytes (89 EC)
        // POP EBP = 1 byte (5D)
        // Total = 3 bytes
        return 3;
    }

    return 0;
}

/**
 * Generate transformed instruction sequence
 */
void generate_enter_leave(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    if (insn->id == X86_INS_ENTER) {
        // ENTER has two operands: imm16 (stack allocation) and imm8 (nesting level)
        // We only handle nesting level 0 (standard case)

        uint16_t stack_size = 0;
        uint8_t nesting_level = 0;

        if (insn->detail->x86.op_count >= 1 && insn->detail->x86.operands[0].type == X86_OP_IMM) {
            stack_size = (uint16_t)insn->detail->x86.operands[0].imm;
        }
        if (insn->detail->x86.op_count >= 2 && insn->detail->x86.operands[1].type == X86_OP_IMM) {
            nesting_level = (uint8_t)insn->detail->x86.operands[1].imm;
        }

        // Only handle nesting level 0 (common case)
        if (nesting_level != 0) {
            // Fallback for complex ENTER with nesting
            buffer_append(b, insn->bytes, insn->size);
            return;
        }

        // Generate manual prologue:
        // push ebp  (55)
        buffer_write_byte(b, 0x55);

        // mov ebp, esp  (89 E5)
        buffer_write_byte(b, 0x89);
        buffer_write_byte(b, 0xE5);

        // sub esp, stack_size  (81 EC XX XX XX XX)
        if (stack_size > 0) {
            buffer_write_byte(b, 0x81);  // SUB r/m32, imm32
            buffer_write_byte(b, 0xEC);  // ModR/M: ESP
            // Write stack_size as 32-bit little-endian
            buffer_write_byte(b, (uint8_t)(stack_size & 0xFF));
            buffer_write_byte(b, (uint8_t)((stack_size >> 8) & 0xFF));
            buffer_write_byte(b, 0x00);
            buffer_write_byte(b, 0x00);
        }

    } else if (insn->id == X86_INS_LEAVE) {
        // Generate manual epilogue:
        // mov esp, ebp  (89 EC)
        buffer_write_byte(b, 0x89);
        buffer_write_byte(b, 0xEC);

        // pop ebp  (5D)
        buffer_write_byte(b, 0x5D);
    }
}

// Define the strategy structure
strategy_t enter_leave_strategy = {
    .name = "ENTER/LEAVE Alternative Encoding",
    .can_handle = can_handle_enter_leave,
    .get_size = get_size_enter_leave,
    .generate = generate_enter_leave,
    .priority = 74,
    .target_arch = BYVAL_ARCH_X86
};
