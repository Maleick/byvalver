/*
 * Robust Jump Resolution Strategy Header
 *
 * Provides function declarations for robust jump/call resolution
 * that properly handles external targets and prevents null byte introduction.
 */

#ifndef ROBUST_JUMP_STRATEGIES_H
#define ROBUST_JUMP_STRATEGIES_H

/* Registration function for robust jump resolution strategies */
void register_robust_jump_strategies();

#endif /* ROBUST_JUMP_STRATEGIES_H */