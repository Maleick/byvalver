/*
 * BYVALVER - Mixed Arithmetic Base Obfuscation (Priority 73)
 *
 * Expresses immediate values using complex arithmetic expressions involving
 * different bases and multi-step calculations. This provides:
 * - Constant hiding (immediate values are not directly visible)
 * - Signature evasion (magic numbers and constants are obscured)
 * - Code expansion (single MOV becomes multiple instructions)
 * - Analyst confusion (requires manual calculation to understand values)
 *
 * Example transformations:
 *   MOV EAX, 0x0B → XOR EAX, EAX; MOV AL, 0x08; ADD AL, 0x03
 *   MOV EBX, 0x10 → MOV EBX, 0x20; SHR EBX, 1
 */

#ifndef MIXED_ARITHMETIC_BASE_OBFUSCATION_H
#define MIXED_ARITHMETIC_BASE_OBFUSCATION_H

void register_mixed_arithmetic_base_obfuscation();

#endif // MIXED_ARITHMETIC_BASE_OBFUSCATION_H
