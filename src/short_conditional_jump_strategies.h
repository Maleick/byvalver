/*
 * Short Conditional Jump with 8-bit Displacement Strategy
 *
 * PROBLEM: Conditional jumps with 32-bit displacement can contain null bytes.
 * 
 * SOLUTION: Convert long conditional jumps to short conditional jumps when
 * the target is within -128 to +127 bytes, using 8-bit displacement instead of 32-bit.
 *
 * FREQUENCY: Common in shellcode to avoid null bytes in conditional jump offsets
 * PRIORITY: 85 (High - essential for jump offset null elimination)
 */

#ifndef SHORT_CONDITIONAL_JUMP_STRATEGIES_H
#define SHORT_CONDITIONAL_JUMP_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the short conditional jump strategies
void register_short_conditional_jump_strategies();

#endif