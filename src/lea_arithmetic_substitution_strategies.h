#ifndef LEA_ARITHMETIC_SUBSTITUTION_STRATEGIES_H
#define LEA_ARITHMETIC_SUBSTITUTION_STRATEGIES_H

#include "strategy.h"

// Function declarations for lea_arithmetic_substitution_strategies.c
int can_handle_lea_arithmetic_substitution(cs_insn *insn);
size_t get_size_lea_arithmetic_substitution(cs_insn *insn);
void generate_lea_arithmetic_substitution(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t lea_arithmetic_substitution_strategy;

#endif