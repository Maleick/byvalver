/*
 * SALC-based Zero Flag Strategy
 *
 * PROBLEM: Comparing values to zero can involve instructions with null bytes.
 * 
 * SOLUTION: Use SALC (Set AL on Carry) instruction to manipulate flags and 
 * detect zero values without direct comparison operations that might contain nulls.
 *
 * FREQUENCY: Useful in shellcode for zero-value detection without CMP instructions
 * PRIORITY: 75 (Medium-High - good for flag manipulation without nulls)
 */

#ifndef SALC_ZERO_FLAG_STRATEGIES_H
#define SALC_ZERO_FLAG_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the SALC zero flag strategies
void register_salc_zero_flag_strategies();

#endif