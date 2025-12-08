/*
 * LEA with Complex Addressing for Value Construction Strategy
 *
 * PROBLEM: MOV with immediate values containing null bytes are problematic.
 * 
 * SOLUTION: Use LEA (Load Effective Address) with complex addressing modes
 * to construct values without using immediate operands that contain null bytes.
 *
 * FREQUENCY: Common in shellcode for register initialization without nulls
 * PRIORITY: 80 (High - efficient for arithmetic value construction)
 */

#ifndef LEA_COMPLEX_ADDRESSING_STRATEGIES_H
#define LEA_COMPLEX_ADDRESSING_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the LEA complex addressing strategies
void register_lea_complex_addressing_strategies();

#endif