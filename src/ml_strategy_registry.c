/**
 * @file ml_strategy_registry.c
 * @brief Implementation of stable strategy-to-index registry for ML model
 */

#include "ml_strategy_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global registry instance
static ml_strategy_registry_t g_ml_registry = {0};

/**
 * @brief Initialize the ML strategy registry
 */
int ml_strategy_registry_init(strategy_t** strategies, int strategy_count) {
    if (!strategies || strategy_count <= 0 || strategy_count > ML_MAX_STRATEGIES) {
        fprintf(stderr, "[ML Registry] Invalid parameters: count=%d\n", strategy_count);
        return -1;
    }

    // Clear registry
    memset(&g_ml_registry, 0, sizeof(ml_strategy_registry_t));

    // Register each strategy with a stable index
    for (int i = 0; i < strategy_count; i++) {
        if (!strategies[i]) {
            fprintf(stderr, "[ML Registry] NULL strategy at index %d\n", i);
            continue;
        }

        ml_strategy_entry_t* entry = &g_ml_registry.entries[i];
        entry->strategy = strategies[i];
        entry->stable_index = i;  // Stable sequential index
        entry->is_active = 1;

        // Store name copy for verification (strategy->name is max 64 bytes)
        // Use snprintf to avoid truncation warnings
        snprintf(entry->name_copy, sizeof(entry->name_copy), "%s", strategies[i]->name);

        g_ml_registry.count++;
    }

    g_ml_registry.initialized = 1;
    printf("[ML Registry] Initialized with %d strategies\n", g_ml_registry.count);

    return 0;
}

/**
 * @brief Get stable index for a strategy
 */
int ml_strategy_get_index(strategy_t* strategy) {
    if (!strategy || !g_ml_registry.initialized) {
        return -1;
    }

    // Linear search through registry
    // For ~200 strategies this is acceptable; could optimize with hash table if needed
    for (int i = 0; i < g_ml_registry.count; i++) {
        if (g_ml_registry.entries[i].is_active &&
            g_ml_registry.entries[i].strategy == strategy) {
            return g_ml_registry.entries[i].stable_index;
        }
    }

    // Strategy not found - this shouldn't happen if registry is properly initialized
    fprintf(stderr, "[ML Registry] Strategy '%s' not found in registry\n",
            strategy ? strategy->name : "NULL");
    return -1;
}

/**
 * @brief Get strategy from stable index
 */
strategy_t* ml_strategy_get_by_index(int index) {
    if (!g_ml_registry.initialized || index < 0 || index >= g_ml_registry.count) {
        return NULL;
    }

    ml_strategy_entry_t* entry = &g_ml_registry.entries[index];
    if (entry->is_active && entry->stable_index == index) {
        return entry->strategy;
    }

    return NULL;
}

/**
 * @brief Get all valid strategy indices for an instruction
 */
int ml_strategy_get_applicable_indices(strategy_t** applicable_strategies,
                                        int count,
                                        int* out_indices,
                                        int* out_count) {
    if (!applicable_strategies || !out_indices || !out_count || count <= 0) {
        return -1;
    }

    if (!g_ml_registry.initialized) {
        fprintf(stderr, "[ML Registry] Registry not initialized\n");
        return -1;
    }

    *out_count = 0;

    for (int i = 0; i < count; i++) {
        int index = ml_strategy_get_index(applicable_strategies[i]);
        if (index >= 0) {
            out_indices[(*out_count)++] = index;
        } else {
            fprintf(stderr, "[ML Registry] WARNING: Strategy '%s' has no stable index\n",
                    applicable_strategies[i] ? applicable_strategies[i]->name : "NULL");
        }
    }

    return 0;
}

/**
 * @brief Cleanup the registry
 */
void ml_strategy_registry_cleanup(void) {
    memset(&g_ml_registry, 0, sizeof(ml_strategy_registry_t));
    printf("[ML Registry] Cleaned up\n");
}

/**
 * @brief Get the global registry instance
 */
ml_strategy_registry_t* ml_strategy_registry_get(void) {
    return &g_ml_registry;
}
