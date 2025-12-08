#ifndef ARITHMETIC_FLAG_PRESERVATION_STRATEGIES_H
#define ARITHMETIC_FLAG_PRESERVATION_STRATEGIES_H

#include "strategy.h"

// Arithmetic flag preservation strategy
extern strategy_t arithmetic_flag_preservation_strategy;
extern strategy_t flag_preserving_decomposition_strategy;

// Function to register the arithmetic flag preservation strategies
void register_arithmetic_flag_preservation_strategies();

#endif