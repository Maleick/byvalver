#ifndef PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H
#define PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H

#include "strategy.h"

/*
 * Partial Register Optimization Strategies
 *
 * PURPOSE: Optimize immediate value loading by using 8-bit and 16-bit register
 * portions instead of full 32/64-bit registers to avoid null bytes.
 *
 * PRIORITY: 89 (Very High - Foundational)
 *
 * EXAMPLES:
 *   Original: mov eax, 0x00000042 (B8 42 00 00 00) - 3 null bytes
 *   Optimized: xor eax, eax; mov al, 0x42 (31 C0 B0 42) - 0 null bytes
 *
 *   Original: mov eax, 0x00004200 (B8 00 42 00 00) - 3 null bytes
 *   Optimized: xor eax, eax; mov ah, 0x42 (31 C0 B4 42) - 0 null bytes
 *
 *   Original: mov eax, 0x00001234 (B8 34 12 00 00) - 2 null bytes
 *   Optimized: xor eax, eax; mov ax, 0x1234 (31 C0 66 B8 34 12) - 0 null bytes
 */

// Main strategy functions (the other functions are already implemented internally)
int can_handle_partial_register_optimization(cs_insn *insn);
size_t get_size_partial_register_optimization(cs_insn *insn);
void generate_partial_register_optimization(struct buffer *b, cs_insn *insn);

void register_partial_register_optimization_strategies(void);

#endif /* PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H */
