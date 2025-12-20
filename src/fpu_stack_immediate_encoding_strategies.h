#ifndef FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H
#define FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H

#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/*
 * FPU Stack-Based Immediate Encoding Strategies
 *
 * PURPOSE: Exploit x87 Floating-Point Unit (FPU) stack to encode integer values 
 * and avoid bad characters in GPR operations. Uses FILD, FISTP, and other FPU 
 * operations to store and retrieve values without using immediate values that 
 * contain bad characters.
 *
 * PRIORITY: 76 (Medium)
 *
 * EXAMPLES:
 *   Original: mov eax, 0x12345678 (may contain bad chars)
 *   Optimized: push 0x12345678; fild dword [esp]; fistp dword [esp]; pop eax
 *
 *   Original: mov eax, value1; mov ebx, value2; mov ecx, value3 (multiple bad chars)
 *   Optimized: Use FPU stack to temporarily store values
 */

// Strategy interface functions
int can_handle_fpu_stack_immediate_encoding(cs_insn *insn);
size_t get_size_fpu_stack_immediate_encoding(cs_insn *insn);
void generate_fpu_stack_immediate_encoding(struct buffer *b, cs_insn *insn);

// Registration function
void register_fpu_stack_immediate_encoding_strategies(void);

#endif /* FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H */