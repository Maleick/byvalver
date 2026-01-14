/*
 * ARM Strategy Testing
 * Tests Capstone ARM mode selection and basic strategy functionality
 */

#include <stdio.h>
#include <capstone/capstone.h>
#include "core.h"  // for get_capstone_arch_mode
#include "strategy.h"  // for init_strategies

// Test Capstone mode selection
void test_capstone_modes() {
    printf("Testing Capstone architecture mode selection...\n");

    cs_arch arch;
    cs_mode mode;

    // Test x86
    get_capstone_arch_mode(BYVAL_ARCH_X86, &arch, &mode);
    printf("x86: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_X86, CS_MODE_32);

    // Test x64
    get_capstone_arch_mode(BYVAL_ARCH_X64, &arch, &mode);
    printf("x64: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_X86, CS_MODE_64);

    // Test ARM
    get_capstone_arch_mode(BYVAL_ARCH_ARM, &arch, &mode);
    printf("ARM: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_ARM, CS_MODE_ARM);

    // Test ARM64
    get_capstone_arch_mode(BYVAL_ARCH_ARM64, &arch, &mode);
    printf("ARM64: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN);
}

// Test ARM disassembly with synthetic instructions
void test_arm_disassembly() {
    printf("\nTesting ARM disassembly...\n");

    // Simple ARM instructions (little-endian)
    // MOV R0, #255  -> 0xFF 0x00 0xA0 0xE3
    // ADD R1, R0, #1 -> 0x01 0x10 0x80 0xE2
    uint8_t test_code[] = {
        0xFF, 0x00, 0xA0, 0xE3,  // MOV R0, #255
        0x01, 0x10, 0x80, 0xE2   // ADD R1, R0, #1
    };

    csh handle;
    cs_insn *insn;
    size_t count;

    cs_arch arch;
    cs_mode mode;
    get_capstone_arch_mode(BYVAL_ARCH_ARM, &arch, &mode);

    if (cs_open(arch, mode, &handle) != CS_ERR_OK) {
        printf("Failed to open Capstone handle\n");
        return;
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    count = cs_disasm(handle, test_code, sizeof(test_code), 0, 0, &insn);
    printf("Disassembled %zu instructions:\n", count);

    for (size_t i = 0; i < count; i++) {
        printf("  0x%08lx: %s %s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
    }

    cs_free(insn, count);
    cs_close(&handle);
}

// Test strategy registration
void test_strategy_registration() {
    printf("\nTesting strategy registration...\n");

    // Test x86 strategy count
    init_strategies(0, BYVAL_ARCH_X86);
    printf("x86 strategies registered\n");

    // Test ARM strategy count
    init_strategies(0, BYVAL_ARCH_ARM);
    printf("ARM strategies registered\n");

    // Test ARM64 strategy count
    init_strategies(0, BYVAL_ARCH_ARM64);
    printf("ARM64 strategies registered\n");
}

int main() {
    test_capstone_modes();
    test_arm_disassembly();
    test_strategy_registration();

    printf("\nARM testing completed!\n");
    return 0;
}