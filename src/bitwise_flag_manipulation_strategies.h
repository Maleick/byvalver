/*
 * Bitwise Flag Manipulation Strategy
 *
 * PROBLEM: Conditional jumps with null bytes in displacement can be problematic.
 * 
 * SOLUTION: Use SETCC instructions and bitwise operations to avoid conditional 
 * jumps with null displacement by manipulating flags through arithmetic/bitwise ops.
 *
 * FREQUENCY: Useful for avoiding conditional jumps with null displacement
 * PRIORITY: 72 (Medium - good alternative to null-byte conditional jumps)
 */

#ifndef BITWISE_FLAG_MANIPULATION_STRATEGIES_H
#define BITWISE_FLAG_MANIPULATION_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the bitwise flag manipulation strategies
void register_bitwise_flag_manipulation_strategies();

#endif