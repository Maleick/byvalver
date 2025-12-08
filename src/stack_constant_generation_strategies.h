#ifndef STACK_CONSTANT_GENERATION_STRATEGIES_H
#define STACK_CONSTANT_GENERATION_STRATEGIES_H

#include "strategy.h"

// Stack constant generation strategy
extern strategy_t stack_constant_generation_strategy;
extern strategy_t stack_arithmetic_constant_strategy;

// Function to register the stack constant generation strategies
void register_stack_constant_generation_strategies();

#endif