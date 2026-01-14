/*
 * ARM Strategy Declarations
 *
 * Bad-byte elimination strategies for ARM (32-bit) architecture.
 */

#ifndef ARM_STRATEGIES_H
#define ARM_STRATEGIES_H

#include "strategy.h"

// Register all ARM strategies
void register_arm_strategies(void);

// Individual ARM strategy registrations
void register_arm_mov_strategies(void);
void register_arm_arithmetic_strategies(void);
void register_arm_memory_strategies(void);
void register_arm_jump_strategies(void);

#endif /* ARM_STRATEGIES_H */