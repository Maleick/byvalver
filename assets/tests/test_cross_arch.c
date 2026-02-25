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

void init_test_strategies(void) {
    register_strategy(&x86_strategy);
    register_strategy(&x64_strategy);
    register_strategy(&arm_strategy);
    register_strategy(&arm64_strategy);
}

void test_architecture_filtering(void) {
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

int should_warn_arch_mismatch(double target_coverage,
                              double suggested_coverage,
                              size_t suggested_insn_count) {
    if (suggested_insn_count < 2) {
        return 0;
    }
    if (suggested_coverage < 0.55) {
        return 0;
    }
    if ((suggested_coverage - target_coverage) < 0.30) {
        return 0;
    }
    return 1;
}

int test_mismatch_warning_policy(void) {
    struct {
        const char *name;
        double target_coverage;
        double suggested_coverage;
        size_t suggested_insn_count;
        int expected_warn;
    } cases[] = {
        {
            .name = "warn when alternate decode is significantly better",
            .target_coverage = 0.20,
            .suggested_coverage = 0.82,
            .suggested_insn_count = 8,
            .expected_warn = 1
        },
        {
            .name = "do not warn when delta is too small",
            .target_coverage = 0.52,
            .suggested_coverage = 0.70,
            .suggested_insn_count = 8,
            .expected_warn = 0
        },
        {
            .name = "do not warn for weak alternate coverage",
            .target_coverage = 0.05,
            .suggested_coverage = 0.40,
            .suggested_insn_count = 8,
            .expected_warn = 0
        },
        {
            .name = "do not warn with insufficient alternate instruction signal",
            .target_coverage = 0.01,
            .suggested_coverage = 0.95,
            .suggested_insn_count = 1,
            .expected_warn = 0
        }
    };

    int failures = 0;

    printf("\nTesting architecture mismatch warning policy...\n");
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int warn = should_warn_arch_mismatch(
            cases[i].target_coverage,
            cases[i].suggested_coverage,
            cases[i].suggested_insn_count
        );
        int passed = (warn == cases[i].expected_warn);
        printf("%s: %s\n", cases[i].name, passed ? "PASS" : "FAIL");
        if (!passed) {
            failures++;
        }
    }

    return failures;
}

int main(void) {
    int failures = 0;

    test_architecture_filtering();
    failures += test_mismatch_warning_policy();
    printf("\nCross-architecture filtering test completed!\n");

    if (failures != 0) {
        printf("Encountered %d mismatch warning policy failure(s)\n", failures);
        return 1;
    }
    return 0;
}
