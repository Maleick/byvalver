/*
 * INC/DEC Chain Strategy
 *
 * PROBLEM: Loading immediate values that contain null bytes can cause nulls
 * in instruction encodings.
 * 
 * SOLUTION: Build values using chains of INC/DEC operations or other arithmetic
 * operations that don't contain null bytes in their encoding.
 *
 * FREQUENCY: Useful for building small values without using immediate operands
 * PRIORITY: 75 (Medium-High - good for small value construction)
 */

#ifndef INC_DEC_CHAIN_STRATEGIES_H
#define INC_DEC_CHAIN_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the INC/DEC chain strategies
void register_inc_dec_chain_strategies();

#endif