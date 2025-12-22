#ifndef FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H
#define FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * FPU Stack Immediate Encoding Strategy for Bad Character Elimination
 *
 * PURPOSE: Use FPU stack operations to load immediate values that contain
 * bad characters, or use FPU arithmetic to construct values indirectly.
 */

// Strategy interface functions
int can_handle_fpu_immediate_encoding(cs_insn *insn);
size_t get_size_fpu_immediate_encoding(cs_insn *insn);
void generate_fpu_immediate_encoding(struct buffer *b, cs_insn *insn);
int can_handle_fpu_integer_load(cs_insn *insn);
size_t get_size_fpu_integer_load(cs_insn *insn);
void generate_fpu_integer_load(struct buffer *b, cs_insn *insn);

// Registration function
void register_fpu_stack_immediate_encoding_strategies();

#endif /* FPU_STACK_IMMEDIATE_ENCODING_STRATEGIES_H */