#ifndef CALL_POP_IMMEDIATE_STRATEGIES_H
#define CALL_POP_IMMEDIATE_STRATEGIES_H

#include "strategy.h"

// Function declarations for call_pop_immediate_strategies.c
int can_handle_call_pop_immediate(cs_insn *insn);
size_t get_size_call_pop_immediate(cs_insn *insn);
void generate_call_pop_immediate(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t call_pop_immediate_strategy;

#endif