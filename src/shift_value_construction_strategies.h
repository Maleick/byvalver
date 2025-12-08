#ifndef SHIFT_VALUE_CONSTRUCTION_STRATEGIES_H
#define SHIFT_VALUE_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"

// Function declarations for shift_value_construction_strategies.c
int can_handle_shift_value_construction(cs_insn *insn);
size_t get_size_shift_value_construction(cs_insn *insn);
void generate_shift_value_construction(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t shift_value_construction_strategy;

#endif