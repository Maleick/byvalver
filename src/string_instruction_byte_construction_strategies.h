/*
 * String Instruction Byte Construction Strategy
 *
 * PROBLEM: Direct immediate values in MOV instructions can contain null bytes.
 * 
 * SOLUTION: Use LODSB/STOSB string instructions to construct values byte by byte
 * from null-free memory locations to avoid null bytes in immediate operands.
 *
 * FREQUENCY: Useful in shellcode for constructing values that contain null bytes
 * PRIORITY: 75 (High - efficient for byte-by-byte construction)
 */

#ifndef STRING_INSTRUCTION_BYTE_CONSTRUCTION_STRATEGIES_H
#define STRING_INSTRUCTION_BYTE_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the string instruction byte construction strategies
void register_string_instruction_byte_construction_strategies();

#endif