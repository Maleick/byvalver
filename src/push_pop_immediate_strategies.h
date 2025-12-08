/*
 * PUSH-POP Immediate Loading Strategy
 *
 * PROBLEM: Direct MOV with immediate values containing null bytes.
 * 
 * SOLUTION: Use PUSH/POP sequences to load immediate values without 
 * directly encoding them in MOV instructions that might contain nulls.
 *
 * FREQUENCY: Common in shellcode when MOV immediate has null bytes
 * PRIORITY: 77 (High - good for immediate loading without nulls)
 */

#ifndef PUSH_POP_IMMEDIATE_STRATEGIES_H
#define PUSH_POP_IMMEDIATE_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the PUSH-POP immediate loading strategies
void register_push_pop_immediate_strategies();

#endif