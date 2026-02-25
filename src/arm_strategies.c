/*
 * ARM Strategy Implementations
 */

#include "arm_strategies.h"
#include "arm_immediate_encoding.h"
#include "utils.h"
#include "core.h"  // For bad_byte_context_t
#include <capstone/capstone.h>

static uint8_t arm_condition_from_insn(cs_insn *insn) {
    uint32_t raw;
    if (!insn || insn->size < 4) {
        return 0xE;  // AL
    }
    raw = (uint32_t)insn->bytes[0] |
          ((uint32_t)insn->bytes[1] << 8) |
          ((uint32_t)insn->bytes[2] << 16) |
          ((uint32_t)insn->bytes[3] << 24);
    return (uint8_t)((raw >> 28) & 0xF);
}

// ============================================================================
// ARM MOV Strategies
// ============================================================================

/**
 * Strategy: ARM MOV Original
 * Pass through MOV instructions without bad bytes
 */
static int can_handle_arm_mov_original(cs_insn *insn) {
    if (insn->id != ARM_INS_MOV) return 0;
    if (insn->detail->arm.op_count != 2) return 0;

    // Must be register to immediate
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_IMM) {
        return 0;
    }

    // Check if original instruction has bad bytes
    extern bad_byte_context_t g_bad_byte_context;
    return !arm_has_bad_bytes(insn, &g_bad_byte_context.config);
}

static size_t get_size_arm_mov_original(cs_insn *insn) {
    (void)insn;
    return 4;  // ARM instructions are 4 bytes
}

static void generate_arm_mov_original(struct buffer *b, cs_insn *insn) {
    // Just copy the original instruction bytes
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t arm_mov_original_strategy = {
    .name = "arm_mov_original",
    .can_handle = can_handle_arm_mov_original,
    .get_size = get_size_arm_mov_original,
    .generate = generate_arm_mov_original,
    .priority = 10,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM MOV with MVN transformation
 * Transform MOV using MVN (bitwise NOT) when MOV immediate isn't encodable
 */
static int can_handle_arm_mov_mvn(cs_insn *insn) {
    if (insn->id != ARM_INS_MOV) return 0;
    if (insn->detail->arm.op_count != 2) return 0;

    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_IMM) {
        return 0;
    }

    // Check if original has bad bytes
    extern bad_byte_context_t g_bad_byte_context;
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;  // Original is fine
    }

    // Check if MVN transformation would work
    uint32_t imm = (uint32_t)insn->detail->arm.operands[1].imm;
    uint32_t mvn_val;
    return find_arm_mvn_immediate(imm, &mvn_val);
}

static size_t get_size_arm_mov_mvn(cs_insn *insn) {
    (void)insn;
    return 4;  // Single MVN instruction
}

static void generate_arm_mov_mvn(struct buffer *b, cs_insn *insn) {
    uint8_t rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    uint32_t imm = (uint32_t)insn->detail->arm.operands[1].imm;

    uint32_t mvn_val;
    if (!find_arm_mvn_immediate(imm, &mvn_val)) {
        // Fallback to original (shouldn't happen if can_handle passed)
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    int encoded_imm = encode_arm_immediate(mvn_val);
    if (encoded_imm == -1) {
        // Fallback
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Encode MVN instruction: MVN Rd, #imm
    // Condition: AL (0xE), Opcode: MVN (0xF), I=1, S=0
    uint32_t instruction = 0xE3E00000 | (rd << 12) | encoded_imm;

    // Verify no bad bytes
    if (is_bad_byte_free(instruction)) {
        buffer_append(b, (uint8_t*)&instruction, 4);
    } else {
        // Fallback to original
        buffer_append(b, insn->bytes, insn->size);
    }
}

static strategy_t arm_mov_mvn_strategy = {
    .name = "arm_mov_mvn",
    .can_handle = can_handle_arm_mov_mvn,
    .get_size = get_size_arm_mov_mvn,
    .generate = generate_arm_mov_mvn,
    .priority = 12,
    .target_arch = BYVAL_ARCH_ARM
};

// ============================================================================
// ARM ADD Strategies
// ============================================================================

/**
 * Strategy: ARM ADD Original
 * Pass through ADD instructions without bad bytes
 */
static int can_handle_arm_add_original(cs_insn *insn) {
    if (insn->id != ARM_INS_ADD) return 0;

    extern bad_byte_context_t g_bad_byte_context;
    return !arm_has_bad_bytes(insn, &g_bad_byte_context.config);
}

static size_t get_size_arm_add_original(cs_insn *insn) {
    (void)insn;
    return 4;
}

static void generate_arm_add_original(struct buffer *b, cs_insn *insn) {
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t arm_add_original_strategy = {
    .name = "arm_add_original",
    .can_handle = can_handle_arm_add_original,
    .get_size = get_size_arm_add_original,
    .generate = generate_arm_add_original,
    .priority = 10,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM ADD with SUB transformation
 * Transform ADD Rn, Rm, #imm -> SUB Rn, Rm, #-imm (if negative immediate works)
 */
static int can_handle_arm_add_sub(cs_insn *insn) {
    if (insn->id != ARM_INS_ADD) return 0;
    if (insn->detail->arm.op_count != 3) return 0;

    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_REG ||
        insn->detail->arm.operands[2].type != ARM_OP_IMM) {
        return 0;
    }

    // Check if original has bad bytes
    extern bad_byte_context_t g_bad_byte_context;
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    // Check if SUB with negative immediate would work
    uint32_t imm = (uint32_t)insn->detail->arm.operands[2].imm;
    uint32_t neg_imm = (uint32_t)(-(int32_t)imm);
    return is_arm_immediate_encodable(neg_imm);
}

static size_t get_size_arm_add_sub(cs_insn *insn) {
    (void)insn;
    return 4;
}

static void generate_arm_add_sub(struct buffer *b, cs_insn *insn) {
    uint8_t rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    uint8_t rn = get_arm_reg_index(insn->detail->arm.operands[1].reg);
    uint32_t imm = (uint32_t)insn->detail->arm.operands[2].imm;
    uint32_t neg_imm = (uint32_t)(-(int32_t)imm);

    int encoded_imm = encode_arm_immediate(neg_imm);
    if (encoded_imm == -1) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Encode SUB instruction: SUB Rd, Rn, #imm
    // Condition: AL (0xE), Opcode: SUB (0x4), I=1, S=0
    uint32_t instruction = 0xE0400000 | (rd << 12) | (rn << 16) | encoded_imm;

    // Verify no bad bytes
    extern bad_byte_context_t g_bad_byte_context;
    if (is_bad_byte_free(instruction)) {
        buffer_append(b, (uint8_t*)&instruction, 4);
    } else {
        buffer_append(b, insn->bytes, insn->size);
    }
}

static strategy_t arm_add_sub_strategy = {
    .name = "arm_add_sub",
    .can_handle = can_handle_arm_add_sub,
    .get_size = get_size_arm_add_sub,
    .generate = generate_arm_add_sub,
    .priority = 12,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM ADD immediate split
 * Transform ADD Rd, Rn, #imm -> ADD Rd, Rn, #part1 ; ADD Rd, Rd, #part2
 */
static int can_handle_arm_add_split(cs_insn *insn) {
    uint32_t imm, part1, part2;
    uint8_t cond, rd, rn;
    uint32_t instruction1, instruction2;
    extern bad_byte_context_t g_bad_byte_context;

    if (insn->id != ARM_INS_ADD) return 0;
    if (insn->detail->arm.op_count != 3) return 0;
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_REG ||
        insn->detail->arm.operands[2].type != ARM_OP_IMM) {
        return 0;
    }
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    imm = (uint32_t)insn->detail->arm.operands[2].imm;
    if (imm == 0 || !find_arm_addsub_split_immediate(imm, &part1, &part2)) {
        return 0;
    }

    cond = arm_condition_from_insn(insn);
    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].reg);

    if (!encode_arm_dp_immediate(cond, 0x4, rn, rd, part1, 0, &instruction1)) return 0;
    if (!encode_arm_dp_immediate(cond, 0x4, rd, rd, part2, 0, &instruction2)) return 0;

    return is_bad_byte_free(instruction1) && is_bad_byte_free(instruction2);
}

static size_t get_size_arm_add_split(cs_insn *insn) {
    (void)insn;
    return 8;
}

static void generate_arm_add_split(struct buffer *b, cs_insn *insn) {
    uint32_t imm, part1, part2;
    uint8_t cond, rd, rn;
    uint32_t instruction1, instruction2;

    imm = (uint32_t)insn->detail->arm.operands[2].imm;
    if (imm == 0 || !find_arm_addsub_split_immediate(imm, &part1, &part2)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cond = arm_condition_from_insn(insn);
    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].reg);

    if (!encode_arm_dp_immediate(cond, 0x4, rn, rd, part1, 0, &instruction1) ||
        !encode_arm_dp_immediate(cond, 0x4, rd, rd, part2, 0, &instruction2) ||
        !is_bad_byte_free(instruction1) ||
        !is_bad_byte_free(instruction2)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    buffer_append(b, (uint8_t*)&instruction1, 4);
    buffer_append(b, (uint8_t*)&instruction2, 4);
}

static strategy_t arm_add_split_strategy = {
    .name = "arm_add_split",
    .can_handle = can_handle_arm_add_split,
    .get_size = get_size_arm_add_split,
    .generate = generate_arm_add_split,
    .priority = 14,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM SUB immediate split
 * Transform SUB Rd, Rn, #imm -> SUB Rd, Rn, #part1 ; SUB Rd, Rd, #part2
 */
static int can_handle_arm_sub_split(cs_insn *insn) {
    uint32_t imm, part1, part2;
    uint8_t cond, rd, rn;
    uint32_t instruction1, instruction2;
    extern bad_byte_context_t g_bad_byte_context;

    if (insn->id != ARM_INS_SUB) return 0;
    if (insn->detail->arm.op_count != 3) return 0;
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_REG ||
        insn->detail->arm.operands[2].type != ARM_OP_IMM) {
        return 0;
    }
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    if (insn->detail->arm.operands[2].imm < 0) {
        return 0;  // keep scope conservative for this phase
    }
    imm = (uint32_t)insn->detail->arm.operands[2].imm;
    if (imm == 0 || !find_arm_addsub_split_immediate(imm, &part1, &part2)) {
        return 0;
    }

    cond = arm_condition_from_insn(insn);
    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].reg);

    if (!encode_arm_dp_immediate(cond, 0x2, rn, rd, part1, 0, &instruction1)) return 0;
    if (!encode_arm_dp_immediate(cond, 0x2, rd, rd, part2, 0, &instruction2)) return 0;

    return is_bad_byte_free(instruction1) && is_bad_byte_free(instruction2);
}

static size_t get_size_arm_sub_split(cs_insn *insn) {
    (void)insn;
    return 8;
}

static void generate_arm_sub_split(struct buffer *b, cs_insn *insn) {
    uint32_t imm, part1, part2;
    uint8_t cond, rd, rn;
    uint32_t instruction1, instruction2;

    if (insn->detail->arm.operands[2].imm < 0) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }
    imm = (uint32_t)insn->detail->arm.operands[2].imm;
    if (imm == 0 || !find_arm_addsub_split_immediate(imm, &part1, &part2)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cond = arm_condition_from_insn(insn);
    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].reg);

    if (!encode_arm_dp_immediate(cond, 0x2, rn, rd, part1, 0, &instruction1) ||
        !encode_arm_dp_immediate(cond, 0x2, rd, rd, part2, 0, &instruction2) ||
        !is_bad_byte_free(instruction1) ||
        !is_bad_byte_free(instruction2)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    buffer_append(b, (uint8_t*)&instruction1, 4);
    buffer_append(b, (uint8_t*)&instruction2, 4);
}

static strategy_t arm_sub_split_strategy = {
    .name = "arm_sub_split",
    .can_handle = can_handle_arm_sub_split,
    .get_size = get_size_arm_sub_split,
    .generate = generate_arm_sub_split,
    .priority = 14,
    .target_arch = BYVAL_ARCH_ARM
};

// ============================================================================
// ARM LDR/STR Strategies
// ============================================================================

/**
 * Strategy: ARM LDR Original
 * Pass through LDR instructions without bad bytes
 */
static int can_handle_arm_ldr_original(cs_insn *insn) {
    if (insn->id != ARM_INS_LDR) return 0;

    extern bad_byte_context_t g_bad_byte_context;
    return !arm_has_bad_bytes(insn, &g_bad_byte_context.config);
}

static size_t get_size_arm_ldr_original(cs_insn *insn) {
    (void)insn;
    return 4;
}

static void generate_arm_ldr_original(struct buffer *b, cs_insn *insn) {
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t arm_ldr_original_strategy = {
    .name = "arm_ldr_original",
    .can_handle = can_handle_arm_ldr_original,
    .get_size = get_size_arm_ldr_original,
    .generate = generate_arm_ldr_original,
    .priority = 10,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM STR Original
 * Pass through STR instructions without bad bytes
 */
static int can_handle_arm_str_original(cs_insn *insn) {
    if (insn->id != ARM_INS_STR) return 0;

    extern bad_byte_context_t g_bad_byte_context;
    return !arm_has_bad_bytes(insn, &g_bad_byte_context.config);
}

static size_t get_size_arm_str_original(cs_insn *insn) {
    (void)insn;
    return 4;
}

static void generate_arm_str_original(struct buffer *b, cs_insn *insn) {
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t arm_str_original_strategy = {
    .name = "arm_str_original",
    .can_handle = can_handle_arm_str_original,
    .get_size = get_size_arm_str_original,
    .generate = generate_arm_str_original,
    .priority = 10,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM LDR displacement split
 * Transform LDR Rd, [Rn, #disp] -> ADD/SUB Rd, Rn, #pre ; LDR Rd, [Rd, #residual]
 */
static int can_handle_arm_ldr_displacement_split(cs_insn *insn) {
    int32_t displacement, pre_adjust, residual;
    uint32_t pre_magnitude;
    uint8_t pre_opcode, cond, rd, rn;
    uint32_t instruction1, instruction2;
    extern bad_byte_context_t g_bad_byte_context;

    if (insn->id != ARM_INS_LDR) return 0;
    if (insn->detail->arm.op_count != 2) return 0;
    if (insn->detail->arm.writeback) return 0;  // conservative scope
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_MEM) {
        return 0;
    }
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    displacement = (int32_t)insn->detail->arm.operands[1].mem.disp;
    if (displacement == 0) return 0;

    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].mem.base);
    if (rd == rn) return 0;  // avoid clobbering base in same register case

    if (!plan_arm_displacement_rewrite(displacement, &pre_adjust, &residual, &pre_magnitude, &pre_opcode)) {
        return 0;
    }

    cond = arm_condition_from_insn(insn);
    if (!encode_arm_dp_immediate(cond, pre_opcode, rn, rd, pre_magnitude, 0, &instruction1)) return 0;
    if (!encode_arm_ldr_str_immediate(cond, 1, rd, rd, residual, &instruction2)) return 0;

    return is_bad_byte_free(instruction1) && is_bad_byte_free(instruction2);
}

static size_t get_size_arm_ldr_displacement_split(cs_insn *insn) {
    (void)insn;
    return 8;
}

static void generate_arm_ldr_displacement_split(struct buffer *b, cs_insn *insn) {
    int32_t displacement, pre_adjust, residual;
    uint32_t pre_magnitude;
    uint8_t pre_opcode, cond, rd, rn;
    uint32_t instruction1, instruction2;

    displacement = (int32_t)insn->detail->arm.operands[1].mem.disp;
    rd = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].mem.base);
    if (rd == rn) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    if (!plan_arm_displacement_rewrite(displacement, &pre_adjust, &residual, &pre_magnitude, &pre_opcode)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cond = arm_condition_from_insn(insn);
    if (!encode_arm_dp_immediate(cond, pre_opcode, rn, rd, pre_magnitude, 0, &instruction1) ||
        !encode_arm_ldr_str_immediate(cond, 1, rd, rd, residual, &instruction2) ||
        !is_bad_byte_free(instruction1) ||
        !is_bad_byte_free(instruction2)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    buffer_append(b, (uint8_t*)&instruction1, 4);
    buffer_append(b, (uint8_t*)&instruction2, 4);
}

static strategy_t arm_ldr_displacement_split_strategy = {
    .name = "arm_ldr_displacement_split",
    .can_handle = can_handle_arm_ldr_displacement_split,
    .get_size = get_size_arm_ldr_displacement_split,
    .generate = generate_arm_ldr_displacement_split,
    .priority = 14,
    .target_arch = BYVAL_ARCH_ARM
};

/**
 * Strategy: ARM STR displacement split with scratch register preservation
 * Transform STR Rt, [Rn, #disp] ->
 *   ADD/SUB R12, Rn, #pre
 *   STR Rt, [R12, #residual]
 *   SUB/ADD R12, R12, #pre
 */
static int can_handle_arm_str_displacement_split(cs_insn *insn) {
    int32_t displacement, pre_adjust, residual;
    uint32_t pre_magnitude;
    uint8_t pre_opcode, restore_opcode, cond, rt, rn;
    uint32_t instruction1, instruction2, instruction3;
    const uint8_t scratch = 12;  // R12/IP
    extern bad_byte_context_t g_bad_byte_context;

    if (insn->id != ARM_INS_STR) return 0;
    if (insn->detail->arm.op_count != 2) return 0;
    if (insn->detail->arm.writeback) return 0;  // conservative scope
    if (insn->detail->arm.operands[0].type != ARM_OP_REG ||
        insn->detail->arm.operands[1].type != ARM_OP_MEM) {
        return 0;
    }
    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    displacement = (int32_t)insn->detail->arm.operands[1].mem.disp;
    if (displacement == 0) return 0;

    rt = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].mem.base);
    if (rt == scratch || rn == scratch) {
        return 0;
    }

    if (!plan_arm_displacement_rewrite(displacement, &pre_adjust, &residual, &pre_magnitude, &pre_opcode)) {
        return 0;
    }

    restore_opcode = (pre_opcode == 0x4) ? 0x2 : 0x4;
    cond = arm_condition_from_insn(insn);

    if (!encode_arm_dp_immediate(cond, pre_opcode, rn, scratch, pre_magnitude, 0, &instruction1)) return 0;
    if (!encode_arm_ldr_str_immediate(cond, 0, scratch, rt, residual, &instruction2)) return 0;
    if (!encode_arm_dp_immediate(cond, restore_opcode, scratch, scratch, pre_magnitude, 0, &instruction3)) return 0;

    return is_bad_byte_free(instruction1) &&
           is_bad_byte_free(instruction2) &&
           is_bad_byte_free(instruction3);
}

static size_t get_size_arm_str_displacement_split(cs_insn *insn) {
    (void)insn;
    return 12;
}

static void generate_arm_str_displacement_split(struct buffer *b, cs_insn *insn) {
    int32_t displacement, pre_adjust, residual;
    uint32_t pre_magnitude;
    uint8_t pre_opcode, restore_opcode, cond, rt, rn;
    uint32_t instruction1, instruction2, instruction3;
    const uint8_t scratch = 12;

    displacement = (int32_t)insn->detail->arm.operands[1].mem.disp;
    rt = get_arm_reg_index(insn->detail->arm.operands[0].reg);
    rn = get_arm_reg_index(insn->detail->arm.operands[1].mem.base);
    if (rt == scratch || rn == scratch) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    if (!plan_arm_displacement_rewrite(displacement, &pre_adjust, &residual, &pre_magnitude, &pre_opcode)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    restore_opcode = (pre_opcode == 0x4) ? 0x2 : 0x4;
    cond = arm_condition_from_insn(insn);
    if (!encode_arm_dp_immediate(cond, pre_opcode, rn, scratch, pre_magnitude, 0, &instruction1) ||
        !encode_arm_ldr_str_immediate(cond, 0, scratch, rt, residual, &instruction2) ||
        !encode_arm_dp_immediate(cond, restore_opcode, scratch, scratch, pre_magnitude, 0, &instruction3) ||
        !is_bad_byte_free(instruction1) ||
        !is_bad_byte_free(instruction2) ||
        !is_bad_byte_free(instruction3)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    buffer_append(b, (uint8_t*)&instruction1, 4);
    buffer_append(b, (uint8_t*)&instruction2, 4);
    buffer_append(b, (uint8_t*)&instruction3, 4);
}

static strategy_t arm_str_displacement_split_strategy = {
    .name = "arm_str_displacement_split",
    .can_handle = can_handle_arm_str_displacement_split,
    .get_size = get_size_arm_str_displacement_split,
    .generate = generate_arm_str_displacement_split,
    .priority = 14,
    .target_arch = BYVAL_ARCH_ARM
};

// ============================================================================
// ARM Branch Strategies
// ============================================================================

/**
 * Strategy: ARM B/BL Original
 * Pass through branch instructions without bad bytes
 */
static int can_handle_arm_branch_original(cs_insn *insn) {
    if (insn->id != ARM_INS_B && insn->id != ARM_INS_BL) return 0;

    extern bad_byte_context_t g_bad_byte_context;
    return !arm_has_bad_bytes(insn, &g_bad_byte_context.config);
}

static size_t get_size_arm_branch_original(cs_insn *insn) {
    (void)insn;
    return 4;
}

static void generate_arm_branch_original(struct buffer *b, cs_insn *insn) {
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t arm_branch_original_strategy = {
    .name = "arm_branch_original",
    .can_handle = can_handle_arm_branch_original,
    .get_size = get_size_arm_branch_original,
    .generate = generate_arm_branch_original,
    .priority = 10,
    .target_arch = BYVAL_ARCH_ARM
};

static int build_arm_branch_conditional_alt(cs_insn *insn,
                                            uint32_t *skip_instruction_out,
                                            uint32_t *branch_instruction_out) {
    uint32_t raw_instruction;
    int32_t original_word_offset, skip_word_offset, rewritten_word_offset;
    uint8_t cond, inverted_cond;
    uint32_t skip_instruction, branch_instruction;

    if (!insn || !skip_instruction_out || !branch_instruction_out) {
        return 0;
    }
    if (insn->id != ARM_INS_B) {
        return 0;
    }
    if (insn->size < 4) {
        return 0;
    }
    if (insn->detail->arm.op_count != 1 || insn->detail->arm.operands[0].type != ARM_OP_IMM) {
        return 0;
    }

    cond = arm_condition_from_insn(insn);
    if (!invert_arm_condition(cond, &inverted_cond)) {
        return 0;
    }

    raw_instruction = (uint32_t)insn->bytes[0] |
                      ((uint32_t)insn->bytes[1] << 8) |
                      ((uint32_t)insn->bytes[2] << 16) |
                      ((uint32_t)insn->bytes[3] << 24);
    if (!decode_arm_branch_offset(raw_instruction, &original_word_offset)) {
        return 0;
    }
    if (!plan_arm_branch_conditional_alt_offsets(original_word_offset,
                                                 &skip_word_offset,
                                                 &rewritten_word_offset)) {
        return 0;
    }

    if (!encode_arm_branch_instruction(inverted_cond, skip_word_offset, &skip_instruction) ||
        !encode_arm_branch_instruction(cond, rewritten_word_offset, &branch_instruction)) {
        return 0;
    }
    if (!is_bad_byte_free(skip_instruction) || !is_bad_byte_free(branch_instruction)) {
        return 0;
    }

    *skip_instruction_out = skip_instruction;
    *branch_instruction_out = branch_instruction;
    return 1;
}

/**
 * Strategy: ARM branch conditional alternative
 * Transform B<cond> target -> B<invcond> skip ; B<cond> target
 *
 * Phase 3 scope guard:
 * - branch-first conditional alternatives only
 * - predicated ALU/memory conditional rewrites are deferred
 */
static int can_handle_arm_branch_conditional_alt(cs_insn *insn) {
    uint32_t skip_instruction, branch_instruction;
    extern bad_byte_context_t g_bad_byte_context;

    if (insn->id != ARM_INS_B) return 0;  // branch-first only (no BL/predicated ALU or memory)

    if (!arm_has_bad_bytes(insn, &g_bad_byte_context.config)) {
        return 0;
    }

    return build_arm_branch_conditional_alt(insn, &skip_instruction, &branch_instruction);
}

static size_t get_size_arm_branch_conditional_alt(cs_insn *insn) {
    (void)insn;
    return 8;
}

static void generate_arm_branch_conditional_alt(struct buffer *b, cs_insn *insn) {
    uint32_t skip_instruction, branch_instruction;

    if (!build_arm_branch_conditional_alt(insn, &skip_instruction, &branch_instruction)) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    buffer_append(b, (uint8_t*)&skip_instruction, 4);
    buffer_append(b, (uint8_t*)&branch_instruction, 4);
}

static strategy_t arm_branch_conditional_alt_strategy = {
    .name = "arm_branch_conditional_alt",
    .can_handle = can_handle_arm_branch_conditional_alt,
    .get_size = get_size_arm_branch_conditional_alt,
    .generate = generate_arm_branch_conditional_alt,
    .priority = 14,
    .target_arch = BYVAL_ARCH_ARM
};

// ============================================================================
// Registration Functions
// ============================================================================

void register_arm_mov_strategies(void) {
    register_strategy(&arm_mov_original_strategy);
    register_strategy(&arm_mov_mvn_strategy);
}

void register_arm_arithmetic_strategies(void) {
    register_strategy(&arm_add_original_strategy);
    register_strategy(&arm_add_sub_strategy);
    register_strategy(&arm_add_split_strategy);
    register_strategy(&arm_sub_split_strategy);
}

void register_arm_memory_strategies(void) {
    register_strategy(&arm_ldr_original_strategy);
    register_strategy(&arm_str_original_strategy);
    register_strategy(&arm_ldr_displacement_split_strategy);
    register_strategy(&arm_str_displacement_split_strategy);
}

void register_arm_jump_strategies(void) {
    // Keep Phase 3 conditional scope branch-first; broader conditional families are deferred.
    register_strategy(&arm_branch_conditional_alt_strategy);
    register_strategy(&arm_branch_original_strategy);
}

void register_arm_strategies(void) {
    register_arm_mov_strategies();
    register_arm_arithmetic_strategies();
    register_arm_memory_strategies();
    register_arm_jump_strategies();
}
