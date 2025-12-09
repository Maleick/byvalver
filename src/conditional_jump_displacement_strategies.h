#ifndef CONDITIONAL_JUMP_DISPLACEMENT_STRATEGIES_H
#define CONDITIONAL_JUMP_DISPLACEMENT_STRATEGIES_H

#include "strategy.h"

// Conditional Jump Displacement Strategies
// Handles conditional jumps that contain null bytes in displacement or encoding

// Function declarations for conditional_jump_displacement_strategies.c
int can_handle_conditional_jump_displacement(cs_insn *insn);
int can_handle_short_conditional_jump_with_nulls(cs_insn *insn);
int can_handle_conditional_jump_alternative(cs_insn *insn);
size_t get_size_conditional_jump_displacement(cs_insn *insn);
size_t get_size_short_conditional_jump_with_nulls(cs_insn *insn);
size_t get_size_conditional_jump_alternative(cs_insn *insn);
void generate_conditional_jump_displacement(struct buffer *b, cs_insn *insn);
void generate_short_conditional_jump_with_nulls(struct buffer *b, cs_insn *insn);
void generate_conditional_jump_alternative(struct buffer *b, cs_insn *insn);

// Strategy definitions
extern strategy_t conditional_jump_displacement_strategy;
extern strategy_t short_conditional_jump_with_nulls_strategy;
extern strategy_t conditional_jump_alternative_strategy;

// Registration function
void register_conditional_jump_displacement_strategies();

#endif