#ifndef PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H
#define PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * Partial Register Optimization Strategy for Bad Character Elimination
 *
 * PURPOSE: Optimize partial register operations that may contain bad
 * characters in their encodings, replacing them with full register
 * operations or alternative encodings.
 */

// Strategy interface functions
int can_handle_partial_register_optimization(cs_insn *insn);
size_t get_size_partial_register_optimization(cs_insn *insn);
void generate_partial_register_optimization(struct buffer *b, cs_insn *insn);
int can_handle_partial_register_dependency(cs_insn *insn);
size_t get_size_partial_register_dependency(cs_insn *insn);
void generate_partial_register_dependency(struct buffer *b, cs_insn *insn);

// Registration function
void register_partial_register_optimization_strategies();

#endif /* PARTIAL_REGISTER_OPTIMIZATION_STRATEGIES_H */