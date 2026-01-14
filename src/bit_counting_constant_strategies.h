#ifndef BIT_COUNTING_CONSTANT_STRATEGIES_H
#define BIT_COUNTING_CONSTANT_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * POPCNT/LZCNT/TZCNT Bit Counting for Constant Generation Strategy
 *
 * PURPOSE: Use modern CPU bit-counting instructions to generate constant values
 * as an alternative to direct MOV immediates, avoiding bad bytes.
 *
 * TECHNIQUE:
 * Modern CPUs (SSE4.2+, BMI1+) provide bit-counting instructions:
 * - POPCNT (F3 0F B8): Population count - count number of set bits
 * - LZCNT (F3 0F BD): Leading zero count
 * - TZCNT (F3 0F BC): Trailing zero count
 *
 * Use cases:
 * 1. POPCNT: Generate small constants by counting bits in a known value
 *    Example: popcnt eax, 0x1F (5 bits set) => EAX = 5
 *
 * 2. TZCNT: Find position of first set bit (useful for power-of-2 exponents)
 *    Example: tzcnt eax, 0x10000 => EAX = 16 (2^16 = 65536)
 *
 * 3. LZCNT: Count leading zeros (useful for log2-like calculations)
 *    Example: lzcnt eax, 0x100 => EAX = 23 (32 - 23 = 9, close to log2(256))
 *
 * Transformations:
 * mov eax, 5 => mov ebx, 0x1F; popcnt eax, ebx (if 0x1F is bad-byte-free)
 * mov eax, 16 => mov ebx, 0x10000; tzcnt eax, ebx
 *
 * LIMITATIONS:
 * - Requires SSE4.2+ (POPCNT) or BMI1+ (LZCNT/TZCNT) CPUs
 * - Only practical for specific constant patterns
 * - Requires source value to be bad-byte-free
 *
 * PRIORITY: 77 (Low-Medium - modern CPUs only, limited applicability)
 */

// Strategy interface functions
int can_handle_bit_counting(cs_insn *insn);
size_t get_size_bit_counting(cs_insn *insn);
void generate_bit_counting(struct buffer *b, cs_insn *insn);

#endif /* BIT_COUNTING_CONSTANT_STRATEGIES_H */
