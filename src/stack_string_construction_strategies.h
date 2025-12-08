#ifndef STACK_STRING_CONSTRUCTION_STRATEGIES_H
#define STACK_STRING_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"

// Function declarations for stack_string_construction_strategies.c
int can_handle_stack_string_construction(cs_insn *insn);
size_t get_size_stack_string_construction(cs_insn *insn);
void generate_stack_string_construction(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t stack_string_construction_strategy;

#endif