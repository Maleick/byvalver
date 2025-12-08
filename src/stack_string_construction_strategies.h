#ifndef STACK_STRING_CONSTRUCTION_STRATEGIES_H
#define STACK_STRING_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"

// Stack string construction strategy
extern strategy_t stack_string_construction_strategy;
extern strategy_t stack_multi_string_construction_strategy;

// Function to register the stack string construction strategies
void register_stack_string_construction_strategies();

#endif