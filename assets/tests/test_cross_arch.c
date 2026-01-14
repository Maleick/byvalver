/*
 * Cross-Architecture Strategy Validation Test
 * Tests that strategies are properly filtered by architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplified strategy structure
typedef enum {
    BYVAL_ARCH_X86 = 0,
    BYVAL_ARCH_X64 = 1,
    BYVAL_ARCH_ARM = 2,
    BYVAL_ARCH_ARM64 = 3
} byval_arch_t;

typedef struct {
    char name[64];
    int (*can_handle)(void *insn);  // Simplified
    size_t (*get_size)(void *insn);
    void (*generate)(void *b, void *insn);
    int priority;
    byval_arch_t target_arch;
} strategy_t;

// Global strategy registry
#define MAX_STRATEGIES 100
strategy_t *strategies[MAX_STRATEGIES];
int strategy_count = 0;

void register_strategy(strategy_t *strategy) {
    if (strategy_count < MAX_STRATEGIES) {
        strategies[strategy_count++] = strategy;
    }
}

int get_strategies_for_arch(byval_arch_t arch, strategy_t **result_strategies) {
    int count = 0;
    for (int i = 0; i < strategy_count; i++) {
        if (strategies[i]->target_arch == arch) {
            result_strategies[count++] = strategies[i];
        }
    }
    return count;
}

// Mock strategies
strategy_t x86_strategy = {
    .name = "x86_test_strategy",
    .target_arch = BYVAL_ARCH_X86
};

strategy_t x64_strategy = {
    .name = "x64_test_strategy",
    .target_arch = BYVAL_ARCH_X64
};

strategy_t arm_strategy = {
    .name = "arm_test_strategy",
    .target_arch = BYVAL_ARCH_ARM
};

strategy_t arm64_strategy = {
    .name = "arm64_test_strategy",
    .target_arch = BYVAL_ARCH_ARM64
};

void init_test_strategies() {
    register_strategy(&x86_strategy);
    register_strategy(&x64_strategy);
    register_strategy(&arm_strategy);
    register_strategy(&arm64_strategy);
}

void test_architecture_filtering() {
    printf("Testing architecture-based strategy filtering...\n");

    init_test_strategies();

    strategy_t *filtered[MAX_STRATEGIES];

    // Test x86 filtering
    int x86_count = get_strategies_for_arch(BYVAL_ARCH_X86, filtered);
    printf("x86 strategies: %d\n", x86_count);
    for (int i = 0; i < x86_count; i++) {
        printf("  %s\n", filtered[i]->name);
    }

    // Test ARM filtering
    int arm_count = get_strategies_for_arch(BYVAL_ARCH_ARM, filtered);
    printf("ARM strategies: %d\n", arm_count);
    for (int i = 0; i < arm_count; i++) {
        printf("  %s\n", filtered[i]->name);
    }

    // Test ARM64 filtering
    int arm64_count = get_strategies_for_arch(BYVAL_ARCH_ARM64, filtered);
    printf("ARM64 strategies: %d\n", arm64_count);
    for (int i = 0; i < arm64_count; i++) {
        printf("  %s\n", filtered[i]->name);
    }

    // Verify counts
    printf("\nValidation:\n");
    printf("x86 count == 1: %d\n", x86_count == 1);
    printf("ARM count == 1: %d\n", arm_count == 1);
    printf("ARM64 count == 1: %d\n", arm64_count == 1);
}

int main() {
    test_architecture_filtering();
    printf("\nCross-architecture filtering test completed!\n");
    return 0;
}