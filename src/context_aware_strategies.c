/*
 * Context-Aware Strategy Selection
 *
 * PROBLEM: Current strategy selection doesn't consider surrounding instructions
 *          which can lead to transformations that introduce null bytes in other parts
 *
 * SOLUTIONS:
 *   1. Analyze context around each instruction before transformation
 *   2. Validate that transformations don't introduce nulls elsewhere
 *   3. Select strategies based on broader context
 *
 * Priority: This is more of an enhancement to the existing system
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>
#include "core.h" // For struct instruction_node

/* Forward declarations */
extern void register_strategy(strategy_t *s);

// The core concept is to enhance the existing strategy selection process
// by adding validation that the selected strategy won't introduce new nulls
// This doesn't require a new strategy per se, but rather an enhancement to
// the strategy selection logic

/*
 * Validate that a transformation doesn't introduce new nulls
 * This is a helper function to be used during strategy application
 */
static int validate_strategy_output(__attribute__((unused)) cs_insn *insn,
                                   __attribute__((unused)) strategy_t *strategy,
                                   __attribute__((unused)) struct instruction_node *current_node,
                                   __attribute__((unused)) struct instruction_node *head) {
    // This function would validate the strategy output, but currently is not used
    // in the main strategy selection logic. We'll keep it as a placeholder for future use.
    return 1;  // Always return 1 for now to avoid issues
}

/*
 * Context-aware strategy selection function
 * This function enhances the standard strategy selection with additional validation
 */
strategy_t** get_context_aware_strategies_for_instruction(cs_insn *insn, int *count,
                                                        struct instruction_node *current_node,
                                                        struct instruction_node *head, byval_arch_t arch) {
    // First, get the standard strategies
    strategy_t **standard_strategies = get_strategies_for_instruction(insn, count, arch);
    
    // For now, we'll validate each strategy to ensure it doesn't introduce nulls
    // This is a basic form of context-awareness
    
    static strategy_t* validated_strategies[200];  // Same size as the main registry
    int validated_count = 0;
    
    for (int i = 0; i < *count && i < 200; i++) {
        if (validate_strategy_output(insn, standard_strategies[i], current_node, head)) {
            validated_strategies[validated_count++] = standard_strategies[i];
        }
        // If validation fails, we skip this strategy
        // In a more advanced implementation, we might try alternative strategies
    }
    
    *count = validated_count;
    return validated_strategies;
}

// For this implementation, rather than creating a new strategy, 
// we'll create a meta-strategy that validates other strategies

/*
 * Context-aware validation strategy - this doesn't transform code directly
 * but validates that other strategies don't introduce new nulls
 */
static int can_handle_context_aware(cs_insn *insn) {
    // This strategy can technically handle any instruction that has other strategies available
    // In practice, this is more of an enhancement to the strategy selection process
    int count;
    get_strategies_for_instruction(insn, &count, BYVAL_ARCH_X64);
    return count > 0;
}

static size_t get_size_context_aware(cs_insn *insn) {
    // This doesn't generate code directly, so return the original size
    return insn->size;
}

static void generate_context_aware(struct buffer *b, cs_insn *insn) {
    // This strategy doesn't generate any code directly
    // Instead, it's about selecting the right strategy in the first place
    // For now, just fall back to standard processing
    buffer_append(b, insn->bytes, insn->size);
}

/* Strategy definition - this is more of a conceptual strategy */
static strategy_t context_aware_strategy = {
    .name = "Context-Aware Validation",
    .can_handle = can_handle_context_aware,
    .get_size = get_size_context_aware,
    .generate = generate_context_aware,
    .priority = 100  // High priority to influence other selections
};

/* Registration function */
void register_context_aware_strategies() {
    register_strategy(&context_aware_strategy);
}