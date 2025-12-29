/*
 * SIMD XMM Register Immediate Loading Strategy
 *
 * PROBLEM: MOV immediate instructions may contain bad bytes.
 *
 * SOLUTION: Use SIMD XMM registers as an alternative data path to load
 * and transfer values, particularly for zero initialization.
 *
 * Example:
 * Original: mov eax, 0  (may encode with nulls: B8 00 00 00 00)
 * Transform: pxor xmm0, xmm0; movd eax, xmm0
 */

#include "simd_xmm_register_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Check if this strategy can handle the instruction
 */
int can_handle_simd_xmm(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Only handle MOV instructions with immediate to register
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    // Check if it's MOV reg, imm
    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    // Destination must be a 32-bit general-purpose register
    if (dst->type != X86_OP_REG) {
        return 0;
    }

    // Source must be immediate
    if (src->type != X86_OP_IMM) {
        return 0;
    }

    // Get the immediate value
    uint64_t imm = src->imm;

    // Currently only optimize for zero values (most common case)
    // PXOR xmm, xmm is very efficient for zeroing
    if (imm != 0) {
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
size_t get_size_simd_xmm(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Transformation: pxor xmm0, xmm0; movd eax, xmm0
    // PXOR XMM0, XMM0 = 4 bytes (66 0F EF C0)
    // MOVD EAX, XMM0 = 4 bytes (66 0F 7E C0)
    // Total = 8 bytes
    return 8;
}

/**
 * Generate transformed instruction sequence
 */
void generate_simd_xmm(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Get operands
    if (insn->detail->x86.op_count != 2) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    if (src->type != X86_OP_IMM || dst->type != X86_OP_REG) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    uint64_t value = src->imm;
    x86_reg dest_reg = dst->reg;

    // Currently only handle zero initialization
    if (value != 0) {
        // Fallback for non-zero values
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Generate: pxor xmm0, xmm0; movd dest_reg, xmm0

    // PXOR XMM0, XMM0 (66 0F EF C0)
    // Zeros out XMM0 register
    buffer_write_byte(b, 0x66);  // Operand-size override prefix
    buffer_write_byte(b, 0x0F);  // Two-byte opcode escape
    buffer_write_byte(b, 0xEF);  // PXOR opcode
    buffer_write_byte(b, 0xC0);  // ModR/M: XMM0, XMM0

    // MOVD dest_reg, XMM0 (66 0F 7E XX)
    // Moves low 32 bits of XMM0 to destination GPR
    buffer_write_byte(b, 0x66);  // Operand-size override prefix
    buffer_write_byte(b, 0x0F);  // Two-byte opcode escape
    buffer_write_byte(b, 0x7E);  // MOVD opcode (XMM to r32)

    // ModR/M byte: depends on destination register
    // Format: 11 xmm gpr (mode=11, reg=xmm0=000, r/m=gpr)
    // For EAX: C0, ECX: C1, EDX: C2, EBX: C3, ESP: C4, EBP: C5, ESI: C6, EDI: C7
    uint8_t modrm = 0xC0;  // Base: XMM0 to EAX

    // Adjust for different destination registers
    switch (dest_reg) {
        case X86_REG_EAX: modrm = 0xC0; break;
        case X86_REG_ECX: modrm = 0xC1; break;
        case X86_REG_EDX: modrm = 0xC2; break;
        case X86_REG_EBX: modrm = 0xC3; break;
        case X86_REG_ESP: modrm = 0xC4; break;
        case X86_REG_EBP: modrm = 0xC5; break;
        case X86_REG_ESI: modrm = 0xC6; break;
        case X86_REG_EDI: modrm = 0xC7; break;
        default:
            // Unknown register, use EAX as fallback
            modrm = 0xC0;
            break;
    }

    buffer_write_byte(b, modrm);
}

// Define the strategy structure
strategy_t simd_xmm_strategy = {
    .name = "SIMD XMM Register Immediate Loading",
    .can_handle = can_handle_simd_xmm,
    .get_size = get_size_simd_xmm,
    .generate = generate_simd_xmm,
    .priority = 89
};
