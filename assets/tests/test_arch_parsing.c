/*
 * Simple test program to verify --arch flag parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplified config structure for testing
typedef enum {
    BYVAL_ARCH_X86 = 0,
    BYVAL_ARCH_X64 = 1,
    BYVAL_ARCH_ARM = 2,
    BYVAL_ARCH_ARM64 = 3
} byval_arch_t;

typedef struct {
    char *input_file;
    char *output_file;
    byval_arch_t target_arch;
} test_config_t;

// Simplified argument parsing for testing
int parse_test_args(int argc, char *argv[], test_config_t *config) {
    config->target_arch = BYVAL_ARCH_X64; // default

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--arch") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "x86") == 0) {
                config->target_arch = BYVAL_ARCH_X86;
            } else if (strcmp(argv[i+1], "x64") == 0) {
                config->target_arch = BYVAL_ARCH_X64;
            } else if (strcmp(argv[i+1], "arm") == 0) {
                config->target_arch = BYVAL_ARCH_ARM;
            } else if (strcmp(argv[i+1], "arm64") == 0 || strcmp(argv[i+1], "aarch64") == 0) {
                config->target_arch = BYVAL_ARCH_ARM64;
            } else {
                fprintf(stderr, "Invalid architecture: %s\n", argv[i+1]);
                return 1;
            }
            i++; // skip the value
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--arch ARCH] input output\n", argv[0]);
            printf("ARCH: x86, x64, arm, arm64\n");
            return 0;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    test_config_t config = {0};

    if (parse_test_args(argc, argv, &config) != 0) {
        return 1;
    }

    printf("Parsed architecture: ");
    switch (config.target_arch) {
        case BYVAL_ARCH_X86: printf("x86\n"); break;
        case BYVAL_ARCH_X64: printf("x64\n"); break;
        case BYVAL_ARCH_ARM: printf("arm\n"); break;
        case BYVAL_ARCH_ARM64: printf("arm64\n"); break;
        default: printf("unknown\n"); break;
    }

    return 0;
}