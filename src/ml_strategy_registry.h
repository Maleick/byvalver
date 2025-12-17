/**
 * @file ml_strategy_registry.h
 * @brief Stable strategy-to-index registry for ML model
 *
 * This provides a bidirectional mapping between strategies and stable indices
 * that remain consistent across forward pass and backpropagation.
 */

#ifndef ML_STRATEGY_REGISTRY_H
#define ML_STRATEGY_REGISTRY_H

#include "strategy.h"
#include <stdint.h>

#define ML_MAX_STRATEGIES 200

/**
 * @brief Registry entry mapping strategy to stable index
 */
typedef struct {
    strategy_t* strategy;           // Pointer to strategy
    int stable_index;               // Stable index in NN output layer
    char name_copy[64];             // Copy of strategy name for verification
    int is_active;                  // Whether this slot is occupied
} ml_strategy_entry_t;

/**
 * @brief Global strategy registry for ML
 */
typedef struct {
    ml_strategy_entry_t entries[ML_MAX_STRATEGIES];
    int count;                      // Number of registered strategies
    int initialized;                // Whether registry is initialized
} ml_strategy_registry_t;

/**
 * @brief Initialize the ML strategy registry
 * @param strategies Array of strategies from main registry
 * @param strategy_count Number of strategies in array
 * @return 0 on success, -1 on error
 */
int ml_strategy_registry_init(strategy_t** strategies, int strategy_count);

/**
 * @brief Get stable index for a strategy
 * @param strategy Pointer to strategy
 * @return Stable index (0 to ML_MAX_STRATEGIES-1), or -1 if not found
 */
int ml_strategy_get_index(strategy_t* strategy);

/**
 * @brief Get strategy from stable index
 * @param index Stable index
 * @return Pointer to strategy, or NULL if invalid
 */
strategy_t* ml_strategy_get_by_index(int index);

/**
 * @brief Get all valid strategy indices for an instruction
 * @param applicable_strategies Array of strategies that can handle instruction
 * @param count Number of applicable strategies
 * @param out_indices Output array to fill with stable indices
 * @param out_count Output number of indices filled
 * @return 0 on success, -1 on error
 */
int ml_strategy_get_applicable_indices(strategy_t** applicable_strategies,
                                        int count,
                                        int* out_indices,
                                        int* out_count);

/**
 * @brief Cleanup the registry
 */
void ml_strategy_registry_cleanup(void);

/**
 * @brief Get the global registry instance
 * @return Pointer to global registry
 */
ml_strategy_registry_t* ml_strategy_registry_get(void);

#endif /* ML_STRATEGY_REGISTRY_H */
