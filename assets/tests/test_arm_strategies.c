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

int find_arm_addsub_split_immediate(uint32_t target, uint32_t *part1_out, uint32_t *part2_out) {
    if (!part1_out || !part2_out) {
        return 0;
    }
    if (target == 0 || is_arm_immediate_encodable(target)) {
        return 0;
    }

    for (uint32_t part1 = 1; part1 < target; part1++) {
        uint32_t part2 = target - part1;
        if (!is_bad_byte_free(part1) || !is_bad_byte_free(part2)) {
            continue;
        }
        if (is_arm_immediate_encodable(part1) && is_arm_immediate_encodable(part2)) {
            *part1_out = part1;
            *part2_out = part2;
            return 1;
        }
    }
    return 0;
}

int find_arm_displacement_split(int32_t displacement, int32_t *pre_adjust_out, int32_t *residual_out) {
    if (!pre_adjust_out || !residual_out) {
        return 0;
    }
    if (displacement == 0 || displacement < -4095 || displacement > 4095) {
        return 0;
    }

    for (int32_t pre = -256; pre <= 256; pre++) {
        int32_t residual = displacement - pre;
        if (pre == 0) {
            continue;
        }
        if (residual < -4095 || residual > 4095) {
            continue;
        }
        if (!is_bad_byte_free((uint32_t)pre) || !is_bad_byte_free((uint32_t)residual)) {
            continue;
        }
        *pre_adjust_out = pre;
        *residual_out = residual;
        return 1;
    }
    return 0;
}

int decode_arm_branch_offset(uint32_t instruction, int32_t *word_offset_out) {
    int32_t imm24;
    if (!word_offset_out) {
        return 0;
    }

    imm24 = (int32_t)(instruction & 0x00FFFFFFU);
    if (imm24 & 0x00800000) {
        imm24 |= (int32_t)0xFF000000U;
    }
    *word_offset_out = imm24;
    return 1;
}

int encode_arm_branch_instruction(uint8_t cond, int32_t word_offset, uint32_t *instruction_out) {
    if (!instruction_out || cond > 0xF) {
        return 0;
    }
    if (word_offset < -(1 << 23) || word_offset > ((1 << 23) - 1)) {
        return 0;
    }

    *instruction_out = ((uint32_t)cond << 28) |
                       0x0A000000U |
                       ((uint32_t)word_offset & 0x00FFFFFFU);
    return 1;
}

int invert_arm_condition(uint8_t cond, uint8_t *inverted_out) {
    if (!inverted_out) {
        return 0;
    }

    switch (cond & 0xF) {
        case 0x0: *inverted_out = 0x1; return 1;
        case 0x1: *inverted_out = 0x0; return 1;
        case 0x2: *inverted_out = 0x3; return 1;
        case 0x3: *inverted_out = 0x2; return 1;
        case 0x4: *inverted_out = 0x5; return 1;
        case 0x5: *inverted_out = 0x4; return 1;
        case 0x6: *inverted_out = 0x7; return 1;
        case 0x7: *inverted_out = 0x6; return 1;
        case 0x8: *inverted_out = 0x9; return 1;
        case 0x9: *inverted_out = 0x8; return 1;
        case 0xA: *inverted_out = 0xB; return 1;
        case 0xB: *inverted_out = 0xA; return 1;
        case 0xC: *inverted_out = 0xD; return 1;
        case 0xD: *inverted_out = 0xC; return 1;
        default: return 0;
    }
}

int build_arm_branch_conditional_alt(uint32_t original_branch, uint32_t *skip_out, uint32_t *branch_out) {
    uint8_t cond, inverted_cond;
    int32_t word_offset;

    if (!skip_out || !branch_out) {
        return 0;
    }

    cond = (uint8_t)((original_branch >> 28) & 0xF);
    if (!invert_arm_condition(cond, &inverted_cond)) {
        return 0;
    }
    if (!decode_arm_branch_offset(original_branch, &word_offset)) {
        return 0;
    }
    if (!encode_arm_branch_instruction(inverted_cond, 0, skip_out)) {
        return 0;
    }
    return encode_arm_branch_instruction(cond, word_offset - 1, branch_out);
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

    // Split-immediate helper smoke check (target selected for non-trivial split)
    {
        uint32_t split_part1 = 0;
        uint32_t split_part2 = 0;
        uint32_t split_target = 0x123;
        int can_split = find_arm_addsub_split_immediate(split_target, &split_part1, &split_part2);
        printf("find_arm_addsub_split_immediate(0x%X): %d\n", split_target, can_split);
        if (can_split) {
            printf("split parts: 0x%X + 0x%X = 0x%X\n", split_part1, split_part2, split_part1 + split_part2);
        }
    }

    // Displacement split helper smoke check
    {
        int32_t pre_adjust = 0;
        int32_t residual = 0;
        int32_t displacement = 0x123;
        int can_split_disp = find_arm_displacement_split(displacement, &pre_adjust, &residual);
        printf("find_arm_displacement_split(0x%X): %d\n", displacement, can_split_disp);
        if (can_split_disp) {
            printf("displacement split: %d + %d = %d\n", pre_adjust, residual, pre_adjust + residual);
        }
    }

    // Branch-first conditional alternative smoke check
    {
        uint32_t original_branch = 0x1A000004;  // BNE with a small forward offset
        uint32_t skip_branch = 0;
        uint32_t taken_branch = 0;
        int32_t original_offset = 0;
        int32_t taken_offset = 0;
        int can_build_branch_alt = build_arm_branch_conditional_alt(original_branch, &skip_branch, &taken_branch);
        printf("build_arm_branch_conditional_alt(0x%X): %d\n", original_branch, can_build_branch_alt);
        if (can_build_branch_alt) {
            decode_arm_branch_offset(original_branch, &original_offset);
            decode_arm_branch_offset(taken_branch, &taken_offset);
            printf("branch alt skip: 0x%08X\n", skip_branch);
            printf("branch alt taken: 0x%08X\n", taken_branch);
            printf("branch offsets (orig/taken): %d -> %d\n", original_offset, taken_offset);
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
