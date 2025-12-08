/*
 * XCHG-Based Immediate Construction Strategy
 *
 * PROBLEM: Loading immediate values containing null bytes directly.
 * 
 * SOLUTION: Use XCHG with pre-loaded null-free values to construct 
 * immediate values without direct encoding that contains nulls.
 *
 * FREQUENCY: Useful when direct immediate loading has null bytes
 * PRIORITY: 70 (Medium - good for specific immediate construction cases)
 */

#ifndef XCHG_IMMEDIATE_CONSTRUCTION_STRATEGIES_H
#define XCHG_IMMEDIATE_CONSTRUCTION_STRATEGIES_H

#include "strategy.h"
#include <capstone/capstone.h>

// Register the XCHG immediate construction strategies
void register_xchg_immediate_construction_strategies();

#endif