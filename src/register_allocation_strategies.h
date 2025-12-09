#ifndef REGISTER_ALLOCATION_STRATEGIES_H
#define REGISTER_ALLOCATION_STRATEGIES_H

#include "strategy.h"

// Register Allocation Strategies for Null Avoidance
// Handles register remapping to avoid null-byte patterns by selecting
// alternative registers that naturally don't introduce nulls

// Function declarations for register_allocation_strategies.c
int can_handle_register_remap_nulls(cs_insn *insn);
int can_handle_mov_register_remap(cs_insn *insn);
int can_handle_contextual_register_swap(cs_insn *insn);
size_t get_size_register_remap_nulls(cs_insn *insn);
size_t get_size_mov_register_remap(cs_insn *insn);
size_t get_size_contextual_register_swap(cs_insn *insn);
void generate_register_remap_nulls(struct buffer *b, cs_insn *insn);
void generate_mov_register_remap(struct buffer *b, cs_insn *insn);
void generate_contextual_register_swap(struct buffer *b, cs_insn *insn);

// Strategy definitions
extern strategy_t register_remap_nulls_strategy;
extern strategy_t mov_register_remap_strategy;
extern strategy_t contextual_register_swap_strategy;

// Registration function
void register_register_allocation_strategies();

#endif