#ifndef STRING_INSTRUCTION_BYTE_CONSTRUCTION_STRATEGIES_H
#define STRING_INSTRUCTION_BYTE_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"

// String instruction byte construction strategy
extern strategy_t string_instruction_byte_construction_strategy;
extern strategy_t stosd_construction_strategy;

// Function to register the string instruction byte construction strategies
void register_string_instruction_byte_construction_strategies();

#endif