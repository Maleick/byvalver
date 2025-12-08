#ifndef SALC_CONDITIONAL_FLAG_STRATEGIES_H
#define SALC_CONDITIONAL_FLAG_STRATEGIES_H

#include "strategy.h"

// Function declarations for salc_conditional_flag_strategies.c
int can_handle_salc_conditional_flag(cs_insn *insn);
size_t get_size_salc_conditional_flag(cs_insn *insn);
void generate_salc_conditional_flag(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t salc_conditional_flag_strategy;

#endif