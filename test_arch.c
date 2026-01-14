#include <stdio.h>
#include <stdlib.h>
#include "cli.h"

int main(int argc, char *argv[]) {
    byvalver_config_t *config = config_create_default();

    // Test parsing
    char *test_args[] = {"byvalver", "--arch", "arm", "input.bin", "output.bin"};
    int test_argc = 5;

    int result = parse_arguments(test_argc, test_args, config);
    if (result == EXIT_SUCCESS) {
        printf("Architecture parsed: %d\n", config->target_arch);
        printf("Expected BYVAL_ARCH_ARM: %d\n", BYVAL_ARCH_ARM);
    } else {
        printf("Parse failed: %d\n", result);
    }

    config_free(config);
    return 0;
}