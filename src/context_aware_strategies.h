/*
 * Context-Aware Strategy Selection Header
 *
 * Provides function declarations for context-aware strategy selection
 * that considers surrounding instructions to prevent null byte introduction.
 */

#ifndef CONTEXT_AWARE_STRATEGIES_H
#define CONTEXT_AWARE_STRATEGIES_H

#include "strategy.h"

/* Registration function for context-aware strategy selection */
void register_context_aware_strategies();

/* Context-aware strategy selection function */
strategy_t** get_context_aware_strategies_for_instruction(cs_insn *insn, int *count, 
                                                        struct instruction_node *current_node,
                                                        struct instruction_node *head);

#endif /* CONTEXT_AWARE_STRATEGIES_H */