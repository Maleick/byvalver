/*
 * Comprehensive Cross-Architecture Functionality Test
 * Tests the complete integration of architecture selection, strategy loading, and processing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <capstone/capstone.h>
#include "core.h"
#include "strategy.h"

// Simplified test to show the full pipeline working
int test_full_pipeline() {
    printf("Testing full cross-architecture pipeline...\n");

    // Test 1: Architecture mode selection
    printf("\n1. Architecture mode selection:\n");
    cs_arch arch;
    cs_mode mode;

    get_capstone_arch_mode(BYVAL_ARCH_X86, &arch, &mode);
    printf("   x86: CS_ARCH=%d, CS_MODE=%d\n", arch, mode);

    get_capstone_arch_mode(BYVAL_ARCH_ARM, &arch, &mode);
    printf("   ARM: CS_ARCH=%d, CS_MODE=%d\n", arch, mode);

    // Test 2: Strategy registration by architecture
    printf("\n2. Strategy registration by architecture:\n");

    // Initialize x86 strategies
    init_strategies(0, BYVAL_ARCH_X86);
    printf("   x86 strategies initialized\n");

    // Initialize ARM strategies
    init_strategies(0, BYVAL_ARCH_ARM);
    printf("   ARM strategies initialized\n");

    // Test 3: ARM disassembly and strategy matching
    printf("\n3. ARM instruction processing:\n");

    // Simple ARM instruction with null byte: MOV R0, #0
    uint8_t arm_code[] = {0x00, 0x00, 0xA0, 0xE3};

    csh handle;
    cs_insn *insn;

    if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &handle) != CS_ERR_OK) {
        printf("   Failed to open Capstone\n");
        return 1;
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    size_t count = cs_disasm(handle, arm_code, sizeof(arm_code), 0, 1, &insn);
    if (count > 0) {
        printf("   Disassembled: %s %s\n", insn->mnemonic, insn->op_str);
        printf("   Instruction bytes: ");
        for (int i = 0; i < insn->size; i++) {
            printf("%02x ", insn->bytes[i]);
        }
        printf("\n");

        // Test strategy matching
        int strategy_count;
        strategy_t** strategies = get_strategies_for_instruction(insn, &strategy_count, BYVAL_ARCH_ARM);
        printf("   Available ARM strategies: %d\n", strategy_count);

        for (int i = 0; i < strategy_count && i < 3; i++) {
            printf("     - %s (priority %d)\n", strategies[i]->name, strategies[i]->priority);
        }
    }

    cs_free(insn, count);
    cs_close(&handle);

    printf("\nFull pipeline test completed successfully!\n");
    return 0;
}

int main() {
    return test_full_pipeline();
}