#ifndef ADVANCED_STRING_OPERATION_STRATEGIES_H
#define ADVANCED_STRING_OPERATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/*
 * Advanced String Operation Transformation Strategies
 *
 * PURPOSE: Transform string instructions (MOVSB/MOVSW/MOVSD, LODSB/LODSW/LODSD, 
 * STOSB/STOSW/STOSD) with REP prefix that may encode with bad bytes in:
 * - REP prefix combinations
 * - Operand size overrides
 * - Register-based addressing
 *
 * PRIORITY: 85 (High)
 *
 * EXAMPLES:
 *   Original: rep movsb (F3 A4) - may have issues with prefixes
 *   Optimized: Manual loop with mov [edi], [esi]; inc esi; inc edi; dec ecx; jnz loop
 *
 *   Original: lodsd (AD) - may have issues in certain contexts
 *   Optimized: mov eax, [esi]; add esi, 4
 */

// Strategy interface functions
int can_handle_advanced_string_operation(cs_insn *insn);
size_t get_size_advanced_string_operation(cs_insn *insn);
void generate_advanced_string_operation(struct buffer *b, cs_insn *insn);

// Registration function
void register_advanced_string_operation_strategies(void);

#endif /* ADVANCED_STRING_OPERATION_STRATEGIES_H */