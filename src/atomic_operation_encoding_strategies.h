#ifndef ATOMIC_OPERATION_ENCODING_STRATEGIES_H
#define ATOMIC_OPERATION_ENCODING_STRATEGIES_H

#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/*
 * Atomic Operation Encoding Chain Strategies
 *
 * PURPOSE: Transform atomic operations (XADD, CMPXCHG, LOCK prefix) that may 
 * encode with bad characters in:
 * - LOCK prefix (F0h) which may combine with opcodes to form bad characters
 * - Complex ModR/M bytes
 * - Memory displacements containing nulls
 *
 * PRIORITY: 78 (Medium-High)
 *
 * EXAMPLES:
 *   Original: lock xadd [counter], eax (F0 0F C1 04 25 XX XX XX XX)
 *   Optimized: mov temp, [counter]; add temp, eax; mov [counter], temp; mov eax, temp
 *
 *   Original: lock cmpxchg [ptr], ebx (F0 0F B1 1D XX XX XX XX) 
 *   Optimized: cmp eax, [ptr]; jnz fail; mov [ptr], ebx; jmp done; fail: ...
 */

// Strategy interface functions
int can_handle_atomic_operation_encoding(cs_insn *insn);
size_t get_size_atomic_operation_encoding(cs_insn *insn);
void generate_atomic_operation_encoding(struct buffer *b, cs_insn *insn);

// Registration function
void register_atomic_operation_encoding_strategies(void);

#endif /* ATOMIC_OPERATION_ENCODING_STRATEGIES_H */