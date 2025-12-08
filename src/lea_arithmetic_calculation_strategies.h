/*
 * LEA-based Address Calculation Strategy
 *
 * PROBLEM: Arithmetic operations with immediate values containing null bytes.
 * 
 * SOLUTION: Use LEA instruction to perform arithmetic operations (addition,
 * multiplication by 1, 2, 4, or 8) without using immediate operands that 
 * contain null bytes.
 *
 * FREQUENCY: Useful for arithmetic operations without null bytes in results
 * PRIORITY: 78 (High - efficient for arithmetic without immediate nulls)
 */

#ifndef LEA_ARITHMETIC_CALCULATION_STRATEGIES_H
#define LEA_ARITHMETIC_CALCULATION_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the LEA arithmetic calculation strategies
void register_lea_arithmetic_calculation_strategies();

#endif