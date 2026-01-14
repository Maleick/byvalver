/*
 * ARM Strategy Validation Test
 * Tests ARM strategy can_handle and generate functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <capstone/capstone.h>

// Simplified bad byte context for testing
typedef struct {
    uint8_t bad_bytes[256];
    int bad_byte_count;
} bad_byte_config_t;

// Global bad byte context
bad_byte_config_t g_bad_byte_config = {0};

// Check if value has bad bytes
int is_bad_byte_free(uint32_t val) {
    uint8_t *bytes = (uint8_t*)&val;
    for (int i = 0; i < 4; i++) {
        if (g_bad_byte_config.bad_bytes[bytes[i]]) {
            return 0;  // Has bad byte
        }
    }
    return 1;  // No bad bytes
}

// Simplified ARM immediate functions
int is_arm_immediate_encodable(uint32_t value) {
    for (int rotation = 0; rotation < 32; rotation += 2) {
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
        if ((rotated & 0xFFFFFF00) == 0) {
            return 1;
        }
    }
    return 0;
}

int encode_arm_immediate(uint32_t value) {
    for (int rotation = 0; rotation < 32; rotation += 2) {
        uint32_t rotated = (value >> rotation) | (value << (32 - rotation));
        if ((rotated & 0xFFFFFF00) == 0) {
            uint8_t imm8 = rotated & 0xFF;
            uint8_t rot = (rotation / 2) & 0xF;
            return (rot << 8) | imm8;
        }
    }
    return -1;
}

int find_arm_mvn_immediate(uint32_t target, uint32_t *mvn_val_out) {
    uint32_t mvn_target = ~target;
    if (is_arm_immediate_encodable(mvn_target)) {
        *mvn_val_out = mvn_target;
        return 1;
    }
    return 0;
}

uint8_t get_arm_reg_index(arm_reg reg) {
    switch (reg) {
        case ARM_REG_R0: return 0;
        case ARM_REG_R1: return 1;
        default: return 0;
    }
}

// Simplified buffer for testing
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} buffer_t;

void buffer_init(buffer_t *b) {
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;
}

void buffer_append(buffer_t *b, const uint8_t *data, size_t size) {
    if (b->size + size > b->capacity) {
        b->capacity = b->size + size + 16;
        b->data = realloc(b->data, b->capacity);
    }
    memcpy(b->data + b->size, data, size);
    b->size += size;
}

// ARM strategy implementations (simplified)
int can_handle_arm_mov_original(cs_insn *insn) {
    if (insn->id != ARM_INS_MOV) return 0;
    if (insn->detail->arm.op_count != 2) return 0;
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_IMM) {
        return 0;
    }
    // Check if instruction has bad bytes
    return !is_bad_byte_free(*(uint32_t*)insn->bytes);
}

size_t get_size_arm_mov_original(cs_insn *insn) {
    (void)insn;
    return 4;
}

void generate_arm_mov_original(buffer_t *b, cs_insn *insn) {
    buffer_append(b, insn->bytes, insn->size);
}

int can_handle_arm_mov_mvn(cs_insn *insn) {
    if (!can_handle_arm_mov_original(insn)) return 0;

    uint32_t imm = (uint32_t)insn->detail->arm.operands[1].imm;
    uint32_t mvn_val;
    return find_arm_mvn_immediate(imm, &mvn_val);
}

void generate_arm_mov_mvn(buffer_t *b, cs_insn *insn) {
    uint8_t rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    uint32_t imm = (uint32_t)insn->detail->arm.operands[1].imm;

    uint32_t mvn_val;
    if (!find_arm_mvn_immediate(imm, &mvn_val)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    int encoded_imm = encode_arm_immediate(mvn_val);
    if (encoded_imm == -1) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Encode MVN instruction
    uint32_t instruction = 0xE3E00000 | (rd << 12) | encoded_imm;
    buffer_append(b, (uint8_t*)&instruction, 4);
}

void test_arm_strategies() {
    printf("Testing ARM strategies...\n");

    // Set up bad bytes (null byte)
    g_bad_byte_config.bad_bytes[0] = 1;
    g_bad_byte_config.bad_byte_count = 1;

    // Test instruction: MOV R0, #0 (has null byte)
    uint8_t test_code[] = {0x00, 0x00, 0xA0, 0xE3};  // MOV R0, #0

    csh handle;
    cs_insn *insn;

    if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &handle) != CS_ERR_OK) {
        printf("Failed to open Capstone\n");
        return;
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    size_t count = cs_disasm(handle, test_code, sizeof(test_code), 0, 1, &insn);
    if (count == 0) {
        printf("Failed to disassemble\n");
        cs_close(&handle);
        return;
    }

    printf("Instruction: %s %s\n", insn->mnemonic, insn->op_str);
    printf("Has bad bytes: %d\n", !is_bad_byte_free(*(uint32_t*)insn->bytes));

    // Test strategies
    printf("can_handle_arm_mov_original: %d\n", can_handle_arm_mov_original(insn));
    printf("can_handle_arm_mov_mvn: %d\n", can_handle_arm_mov_mvn(insn));

    // Test generation
    buffer_t output;
    buffer_init(&output);

    if (can_handle_arm_mov_mvn(insn)) {
        printf("Generating MVN transformation...\n");
        generate_arm_mov_mvn(&output, insn);

        printf("Output size: %zu bytes\n", output.size);
        if (output.size >= 4) {
            uint32_t result = *(uint32_t*)output.data;
            printf("Generated instruction: 0x%08X\n", result);
            printf("Has bad bytes: %d\n", !is_bad_byte_free(result));
        }
    }

    cs_free(insn, count);
    cs_close(&handle);
    free(output.data);
}

int main() {
    test_arm_strategies();
    printf("\nARM strategy test completed!\n");
    return 0;
}