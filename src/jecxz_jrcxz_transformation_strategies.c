/*
 * JECXZ/JRCXZ Zero-Test Jump Transformation Strategy
 *
 * PROBLEM: JECXZ/JRCXZ instructions use 8-bit signed displacement which may
 * contain bad bytes.
 *
 * SOLUTION: Replace with TEST + JZ sequence that achieves the same result.
 *
 * Example:
 * Original: jecxz target  (E3 XX where XX is displacement)
 * Transform: test ecx, ecx; jz target  (85 C9 74 XX)
 */

#include "jecxz_jrcxz_transformation_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Check if this strategy can handle the instruction
 */
int can_handle_jecxz_jrcxz(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if instruction is JECXZ or JRCXZ
    if (insn->id != X86_INS_JECXZ) {
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
size_t get_size_jecxz_jrcxz(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Transformation: test ecx, ecx; jz target
    // TEST ECX, ECX = 2 bytes (85 C9) for 32-bit
    // JZ rel8 = 2 bytes (74 XX)
    // Total = 4 bytes
    //
    // For x64 with RCX:
    // TEST RCX, RCX = 3 bytes (48 85 C9)
    // JZ rel8 = 2 bytes (74 XX)
    // Total = 5 bytes

    // For simplicity, return 5 bytes (handles both x86 and x64)
    return 5;
}

/**
 * Generate transformed instruction sequence
 */
void generate_jecxz_jrcxz(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Get the target displacement from JECXZ/JRCXZ
    if (insn->detail->x86.op_count != 1) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cs_x86_op *op = &insn->detail->x86.operands[0];
    if (op->type != X86_OP_IMM) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Calculate relative displacement
    // The displacement in JECXZ is relative to the instruction AFTER JECXZ
    int64_t target_addr = op->imm;
    int64_t next_insn_addr = insn->address + insn->size;
    int64_t displacement = target_addr - next_insn_addr;

    // After transformation, we need to adjust the displacement
    // because our instruction sequence is longer
    size_t transform_size = get_size_jecxz_jrcxz(insn);
    int64_t adjusted_disp = displacement - (transform_size - insn->size);

    // Check if displacement fits in 8-bit signed range
    if (adjusted_disp < -128 || adjusted_disp > 127) {
        // Need a 32-bit displacement - use long form jump
        // For now, fallback (could implement 0F 84 for 32-bit JZ)
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    int8_t disp8 = (int8_t)adjusted_disp;

    // Determine if we're in x64 mode by checking address size
    // For simplicity, check if this is likely x64 by instruction mode
    // In real implementation, would check cs_mode or architecture
    int is_x64 = (insn->detail->x86.encoding.modrm_offset > 0);  // Heuristic

    // Generate: TEST ECX/RCX, ECX/RCX; JZ target

    if (is_x64) {
        // For x64: TEST RCX, RCX (48 85 C9)
        buffer_write_byte(b, 0x48);  // REX.W prefix for 64-bit operand
        buffer_write_byte(b, 0x85);  // TEST opcode
        buffer_write_byte(b, 0xC9);  // ModR/M: RCX, RCX
    } else {
        // For x86: TEST ECX, ECX (85 C9)
        buffer_write_byte(b, 0x85);  // TEST opcode
        buffer_write_byte(b, 0xC9);  // ModR/M: ECX, ECX
    }

    // JZ rel8 (74 XX)
    buffer_write_byte(b, 0x74);  // JZ opcode
    buffer_write_byte(b, (uint8_t)disp8);
}

// Define the strategy structure
strategy_t jecxz_jrcxz_strategy = {
    .name = "JECXZ/JRCXZ Zero-Test Jump",
    .can_handle = can_handle_jecxz_jrcxz,
    .get_size = get_size_jecxz_jrcxz,
    .generate = generate_jecxz_jrcxz,
    .priority = 85,
    .target_arch = BYVAL_ARCH_X86
};
