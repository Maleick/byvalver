#ifndef JECXZ_JRCXZ_TRANSFORMATION_STRATEGIES_H
#define JECXZ_JRCXZ_TRANSFORMATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * JECXZ/JRCXZ Zero-Test Jump Transformation Strategy
 *
 * PURPOSE: Replace JECXZ/JRCXZ instructions that have bad characters in their
 * displacement bytes with equivalent TEST + JZ sequences.
 *
 * TECHNIQUE:
 * JECXZ and JRCXZ are conditional jump instructions that test if ECX/RCX is zero:
 * - JECXZ rel8 (E3 cb): Jump if ECX = 0 (x86/x64 compatibility mode)
 * - JRCXZ rel8 (E3 cb): Jump if RCX = 0 (x64 native mode)
 *
 * The displacement is 8-bit signed (-128 to +127) and may contain bad characters.
 *
 * Transformation:
 * JECXZ target (E3 XX) => TEST ECX, ECX; JZ target
 * JRCXZ target (E3 XX) => TEST RCX, RCX; JZ target
 *
 * Benefits:
 * - Eliminates bad characters in displacement byte
 * - TEST instruction sets ZF based on bitwise AND (without modifying operands)
 * - JZ can use longer displacement if needed (or further transformations)
 *
 * Example:
 * Original: jecxz loop_end  (E3 00) - displacement is 0x00 (bad char)
 * Transform: test ecx, ecx; jz loop_end  (85 C9 74 XX)
 *
 * LIMITATIONS:
 * - Size increases from 2 bytes to 4+ bytes
 * - TEST modifies flags (ZF, SF, PF), clears OF and CF
 * - Original JECXZ doesn't modify flags, but transformed version does
 *
 * PRIORITY: 85 (Medium-High - fills gap in jump coverage, common in loops)
 */

// Strategy interface functions
int can_handle_jecxz_jrcxz(cs_insn *insn);
size_t get_size_jecxz_jrcxz(cs_insn *insn);
void generate_jecxz_jrcxz(struct buffer *b, cs_insn *insn);

#endif /* JECXZ_JRCXZ_TRANSFORMATION_STRATEGIES_H */
