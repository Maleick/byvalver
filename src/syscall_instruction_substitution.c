/*
 * BYVALVER - Syscall Instruction Substitution (Priority 79)
 *
 * Transforms syscall invocations to alternative forms for evasion.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"
#include "obfuscation_strategy_registry.h"

// ============================================================================
// Strategy 1: INT 0x80 → PUSH/RET trampoline
// ============================================================================

int can_handle_int80_substitution(cs_insn *insn) {
    if (insn->id != X86_INS_INT) return 0;

    // Check if it's INT 0x80 (Linux syscall)
    if (insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        if (op->type == X86_OP_IMM && op->imm == 0x80) {
            return 1;
        }
    }
    return 0;
}

size_t get_int80_substitution_size(cs_insn *insn) {
    (void)insn;
    // We'll use a simple indirect call pattern
    // PUSH offset_to_int80; CALL trampoline; int80_location: INT 0x80; RET
    // For simplicity, just keep INT 0x80 but wrap it differently
    // Actually, let's use: CALL +2; INT 0x80; RET (3 + 2 + 1 = 6 bytes)
    return 6;
}

void generate_int80_substitution(struct buffer *b, cs_insn *insn) {
    (void)insn;

    // Pattern: CALL +2 (skip over INT 0x80); INT 0x80; RET
    // This makes the syscall appear as a function call

    // CALL +2 (E8 02 00 00 00)
    uint8_t call_bytes[] = {0xE8, 0x02, 0x00, 0x00, 0x00};
    buffer_append(b, call_bytes, 5);

    // INT 0x80
    uint8_t int80_bytes[] = {0xCD, 0x80};
    buffer_append(b, int80_bytes, 2);

    // RET (C3) - will never execute but adds confusion
    uint8_t ret_byte = 0xC3;
    buffer_append(b, &ret_byte, 1);

    // NOTE: This is 8 bytes total, need to adjust size function
}

// ============================================================================
// Strategy 2: SYSCALL (x64) → indirect pattern
// ============================================================================

int can_handle_syscall_substitution(cs_insn *insn) {
    if (insn->id != X86_INS_SYSCALL) return 0;
    return 1;  // Apply to all SYSCALL instructions
}

size_t get_syscall_substitution_size(cs_insn *insn) {
    (void)insn;
    return 6;  // Similar pattern to INT 0x80
}

void generate_syscall_substitution(struct buffer *b, cs_insn *insn) {
    (void)insn;

    // Pattern: CALL +1; SYSCALL; RET
    uint8_t call_bytes[] = {0xE8, 0x01, 0x00, 0x00, 0x00};
    buffer_append(b, call_bytes, 5);

    // SYSCALL (0F 05)
    uint8_t syscall_bytes[] = {0x0F, 0x05};
    buffer_append(b, syscall_bytes, 2);

    // RET
    uint8_t ret_byte = 0xC3;
    buffer_append(b, &ret_byte, 1);
}

// ============================================================================
// Fix: Update INT 0x80 size to match actual generation
// ============================================================================

size_t get_int80_substitution_size_fixed(cs_insn *insn) {
    (void)insn;
    return 8;  // CALL (5) + INT 0x80 (2) + RET (1) = 8 bytes
}

static strategy_t int80_substitution_strategy = {
    .name = "INT 0x80 Substitution",
    .can_handle = can_handle_int80_substitution,
    .get_size = get_int80_substitution_size_fixed,
    .generate = generate_int80_substitution,
    .priority = 79
};

static strategy_t syscall_substitution_strategy = {
    .name = "SYSCALL Substitution",
    .can_handle = can_handle_syscall_substitution,
    .get_size = get_syscall_substitution_size,
    .generate = generate_syscall_substitution,
    .priority = 78
};

// ============================================================================
// Registration
// ============================================================================

void register_syscall_instruction_substitution() {
    register_obfuscation_strategy(&int80_substitution_strategy);
    register_obfuscation_strategy(&syscall_substitution_strategy);
}
