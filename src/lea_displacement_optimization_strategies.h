#ifndef LEA_DISPLACEMENT_OPTIMIZATION_STRATEGIES_H
#define LEA_DISPLACEMENT_OPTIMIZATION_STRATEGIES_H

#include "strategy.h"

// LEA Displacement Optimization Strategies
// Handles LEA (Load Effective Address) instructions that contain null bytes
// in displacement values or addressing modes

// Function declarations for lea_displacement_optimization_strategies.c
int can_handle_lea_displacement_nulls(cs_insn *insn);
int can_handle_lea_problematic_encoding(cs_insn *insn);
size_t get_size_lea_displacement_nulls(cs_insn *insn);
size_t get_size_lea_problematic_encoding(cs_insn *insn);
void generate_lea_displacement_nulls(struct buffer *b, cs_insn *insn);
void generate_lea_problematic_encoding(struct buffer *b, cs_insn *insn);

// Strategy definitions
extern strategy_t lea_displacement_nulls_strategy;
extern strategy_t lea_problematic_encoding_strategy;

// Registration function
void register_lea_displacement_optimization_strategies();

#endif