/*
 * Simple ARM Capstone Test
 * Tests Capstone ARM mode selection and basic disassembly
 */

#include <stdio.h>
#include <capstone/capstone.h>

// Simplified arch enum for testing
typedef enum {
    BYVAL_ARCH_X86 = 0,
    BYVAL_ARCH_X64 = 1,
    BYVAL_ARCH_ARM = 2,
    BYVAL_ARCH_ARM64 = 3
} byval_arch_t;

// Simplified Capstone mode selector
void get_capstone_arch_mode(byval_arch_t arch, cs_arch *cs_arch_out, cs_mode *cs_mode_out) {
    switch (arch) {
        case BYVAL_ARCH_X86:
            *cs_arch_out = CS_ARCH_X86;
            *cs_mode_out = CS_MODE_32;
            break;
        case BYVAL_ARCH_X64:
            *cs_arch_out = CS_ARCH_X86;
            *cs_mode_out = CS_MODE_64;
            break;
        case BYVAL_ARCH_ARM:
            *cs_arch_out = CS_ARCH_ARM;
            *cs_mode_out = CS_MODE_ARM;
            break;
        case BYVAL_ARCH_ARM64:
            *cs_arch_out = CS_ARCH_ARM64;
            *cs_mode_out = CS_MODE_LITTLE_ENDIAN;
            break;
        default:
            *cs_arch_out = CS_ARCH_X86;
            *cs_mode_out = CS_MODE_64;
            break;
    }
}

int main() {
    printf("Testing Capstone ARM mode selection...\n");

    cs_arch arch;
    cs_mode mode;

    // Test ARM
    get_capstone_arch_mode(BYVAL_ARCH_ARM, &arch, &mode);
    printf("ARM: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_ARM, CS_MODE_ARM);

    // Test ARM64
    get_capstone_arch_mode(BYVAL_ARCH_ARM64, &arch, &mode);
    printf("ARM64: arch=%d, mode=%d (expected: %d, %d)\n", arch, mode, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN);

    // Test ARM disassembly
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

    if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &handle) != CS_ERR_OK) {
        printf("Failed to open Capstone handle\n");
        return 1;
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    count = cs_disasm(handle, test_code, sizeof(test_code), 0, 0, &insn);
    printf("Disassembled %zu instructions:\n", count);

    for (size_t i = 0; i < count; i++) {
        printf("  0x%08lx: %s %s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
        if (strcmp(insn[i].mnemonic, "mov") == 0) {
            printf("    -> MOV instruction detected!\n");
        }
    }

    cs_free(insn, count);
    cs_close(&handle);

    printf("\nARM Capstone test completed successfully!\n");
    return 0;
}