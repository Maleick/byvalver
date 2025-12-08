/*
 * SCASB/CMPSB Conditional Operations Strategy
 *
 * PROBLEM: Conditional operations that check for zero/null bytes can contain nulls
 * in immediate values or comparison operands.
 *
 * SOLUTION: Use SCASB/SCASD/CMPSB/CMPSD string instructions to perform comparisons
 * without using immediate values that contain null bytes.
 *
 * FREQUENCY: Useful in shellcode for string operations and comparisons
 * PRIORITY: 75 (High - efficient string operations without null bytes)
 */

#ifndef SCASB_CMPSB_STRATEGIES_H
#define SCASB_CMPSB_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the SCASB/CMPSB conditional operation strategies
void register_scasb_cmpsb_strategies();

#endif