#ifndef SIMD_XMM_REGISTER_STRATEGIES_H
#define SIMD_XMM_REGISTER_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * SIMD XMM Register Immediate Loading Strategy
 *
 * PURPOSE: Use SIMD XMM registers as alternative data path for loading
 * immediate values, avoiding bad characters in standard MOV instructions.
 *
 * TECHNIQUE:
 * XMM registers (128-bit SIMD) can be used to load and transfer data:
 * - PXOR xmm, xmm: Zero out XMM register
 * - MOVD xmm, r32: Move 32-bit value from GPR to XMM[31:0]
 * - MOVD r32, xmm: Move XMM[31:0] to 32-bit GPR
 * - MOVQ xmm, r64: Move 64-bit value (x64)
 *
 * Use cases:
 * 1. Zero initialization: PXOR xmm0, xmm0; MOVD eax, xmm0 => EAX = 0
 * 2. Alternative data path when MOV has bad characters
 * 3. SIMD-based arithmetic for constant construction
 *
 * Transformations:
 * mov eax, 0 => pxor xmm0, xmm0; movd eax, xmm0
 * mov eax, value => mov ebx, value; movd xmm0, ebx; movd eax, xmm0
 *
 * Example:
 * Original: mov eax, 0x00000000  (has null bytes)
 * Transform: pxor xmm0, xmm0; movd eax, xmm0
 *
 * LIMITATIONS:
 * - Requires SSE2+ (available on all modern x86/x64)
 * - Uses XMM registers (may conflict with existing SIMD code)
 * - Larger code size (6+ bytes vs 5 bytes for MOV)
 * - Only beneficial for specific patterns (zeros, small values)
 *
 * PRIORITY: 89 (High - modern CPUs, alternative encoding path)
 */

// Strategy interface functions
int can_handle_simd_xmm(cs_insn *insn);
size_t get_size_simd_xmm(cs_insn *insn);
void generate_simd_xmm(struct buffer *b, cs_insn *insn);

#endif /* SIMD_XMM_REGISTER_STRATEGIES_H */
