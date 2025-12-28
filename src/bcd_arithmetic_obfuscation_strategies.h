#ifndef BCD_ARITHMETIC_OBFUSCATION_STRATEGIES_H
#define BCD_ARITHMETIC_OBFUSCATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * BCD Arithmetic for Obfuscated Constant Generation Strategy
 *
 * PURPOSE: Use Binary-Coded Decimal (BCD) arithmetic instructions to construct
 * constant values in an obfuscated manner, avoiding bad characters in MOV immediates.
 *
 * TECHNIQUE:
 * BCD instructions provide alternative arithmetic operations:
 * - AAM (ASCII Adjust after Multiply): D4 0A - divides AL by 10 (quotient in AH, remainder in AL)
 * - AAD (ASCII Adjust before Division): D5 0A - multiplies AH by 10 and adds AL
 * - DAA (Decimal Adjust after Addition): 27 - adjusts AL after BCD addition
 * - DAS (Decimal Adjust after Subtraction): 2F - adjusts AL after BCD subtraction
 * - AAA (ASCII Adjust after Addition): 37
 * - AAS (ASCII Adjust after Subtraction): 3F
 *
 * Use cases:
 * 1. Construct small constants (0-99) using AAM/AAD decomposition
 * 2. Build values using BCD addition with DAA
 * 3. Obfuscate values to evade signatures
 *
 * Example transformation:
 * mov al, 0x2A (42 decimal)  =>  mov al, 0x42; aam; mov bl, ah; mov al, 10; mul bl; add al, bl
 *
 * LIMITATIONS:
 * - x86 only (invalid in x64 long mode)
 * - Only practical for 8-bit values (0-255)
 * - Primarily for obfuscation, not size optimization
 *
 * PRIORITY: 68 (Very Low - x86 only, complex, niche obfuscation)
 */

// Strategy interface functions
int can_handle_bcd_arithmetic(cs_insn *insn);
size_t get_size_bcd_arithmetic(cs_insn *insn);
void generate_bcd_arithmetic(struct buffer *b, cs_insn *insn);

#endif /* BCD_ARITHMETIC_OBFUSCATION_STRATEGIES_H */
