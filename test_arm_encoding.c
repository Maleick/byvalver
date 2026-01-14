/*
 * ARM Immediate Encoding Test
 */

#include <stdio.h>
#include <stdint.h>

// Simplified ARM immediate encoding functions for testing
int is_arm_immediate_encodable(uint32_t value) {
    // Try all possible rotations (0, 2, 4, ..., 30)
    for (int rotation = 0; rotation < 32; rotation += 2) {
        // Rotate right by 'rotation' bits
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
        // Check if the high 24 bits are zero (fits in 8 bits)
        if ((rotated & 0xFFFFFF00) == 0) {
            return 1;
        }
    }
    return 0;
}

int encode_arm_immediate(uint32_t value) {
    // Try all possible rotations
    for (int rotation = 0; rotation < 32; rotation += 2) {
        // Rotate right by 'rotation' bits
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
        // Check if it fits in 8 bits
        if ((rotated & 0xFFFFFF00) == 0) {
            uint8_t imm8 = rotated & 0xFF;
            uint8_t rot = (rotation / 2) & 0xF;  // Rotation in 2-bit units
            return (rot << 8) | imm8;
        }
    }
    return -1;  // Cannot encode
}

int find_arm_mvn_immediate(uint32_t target, uint32_t *mvn_val_out) {
    uint32_t mvn_target = ~target;  // MVN value
    if (is_arm_immediate_encodable(mvn_target)) {
        *mvn_val_out = mvn_target;
        return 1;
    }
    return 0;
}

void test_arm_immediates() {
    printf("Testing ARM immediate encoding...\n");

    // Test values that should be encodable
    uint32_t test_values[] = {0, 1, 255, 256, 0xFF000000, 0x00FF0000};
    const char *test_names[] = {"0", "1", "255", "256", "0xFF000000", "0x00FF0000"};

    for (int i = 0; i < 6; i++) {
        int encodable = is_arm_immediate_encodable(test_values[i]);
        int encoded = encode_arm_immediate(test_values[i]);
        printf("Value %s (0x%08X): encodable=%d, encoded=0x%04X\n",
               test_names[i], test_values[i], encodable, encoded);
    }

    // Test MVN transformation
    printf("\nTesting MVN transformations...\n");
    uint32_t test_mvn = 0xFFFFFF00;  // ~0xFF = 0xFFFFFF00
    uint32_t mvn_val;
    int can_mvn = find_arm_mvn_immediate(0xFF, &mvn_val);
    printf("MOV R0, #0xFF -> MVN R0, #0x%08X: possible=%d\n", mvn_val, can_mvn);
}

int main() {
    test_arm_immediates();
    printf("\nARM immediate encoding test completed!\n");
    return 0;
}