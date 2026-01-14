/*
 * ARM64 Strategy Declarations
 *
 * Bad-byte elimination strategies for ARM64 (AArch64) architecture.
 */

#ifndef ARM64_STRATEGIES_H
#define ARM64_STRATEGIES_H

#include "strategy.h"

// Register all ARM64 strategies
void register_arm64_strategies(void);

// Individual ARM64 strategy registrations
void register_arm64_mov_strategies(void);
void register_arm64_arithmetic_strategies(void);
void register_arm64_memory_strategies(void);
void register_arm64_jump_strategies(void);

#endif /* ARM64_STRATEGIES_H */