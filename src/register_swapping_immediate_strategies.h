#ifndef REGISTER_SWAPPING_IMMEDIATE_STRATEGIES_H
#define REGISTER_SWAPPING_IMMEDIATE_STRATEGIES_H

#include "strategy.h"

// Function declarations for register_swapping_immediate_strategies.c
int can_handle_register_swapping_immediate(cs_insn *insn);
size_t get_size_register_swapping_immediate(cs_insn *insn);
void generate_register_swapping_immediate(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t register_swapping_immediate_strategy;

#endif