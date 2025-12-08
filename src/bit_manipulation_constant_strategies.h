#ifndef BIT_MANIPULATION_CONSTANT_STRATEGIES_H
#define BIT_MANIPULATION_CONSTANT_STRATEGIES_H

#include "strategy.h"

// Bit manipulation constant strategy
extern strategy_t bit_manipulation_constant_strategy;
extern strategy_t rotation_xor_constant_strategy;

// Function to register the bit manipulation constant strategies
void register_bit_manipulation_constant_strategies();

#endif