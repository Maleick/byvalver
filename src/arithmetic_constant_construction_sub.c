/*
 * BYVALVER - Arithmetic Constant Construction via SUB (Priority 79)
 *
 * Constructs immediate values containing null bytes by starting with a
 * null-free base value and subtracting a null-free offset.
 * Example: To get 32 (0x20), use: MOV BX, 1666; SUB BX, 1634
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"

// ============================================================================
// Helper: Find SUB-based construction for target value
// ============================================================================

static int find_sub_construction(uint32_t target, uint32_t *base, uint32_t *offset) {
    // Try to find null-free base and offset such that: base - offset = target
    // We limit search to reasonable ranges for efficiency

    // For small targets (0-1000), try base values from 1000-2000
    if (target < 1000) {
        for (uint32_t b = 1000; b < 2000; b++) {
            if (!is_null_free(b)) continue;

            uint32_t o = b - target;
            if (o > 0 && o < 2000 && is_null_free(o)) {
                *base = b;
                *offset = o;
                return 1;
            }
        }
    }

    // For larger targets, try different ranges
    if (target < 100000) {
        for (uint32_t b = target + 1000; b < target + 5000; b++) {
            if (!is_null_free(b)) continue;

            uint32_t o = b - target;
            if (o > 0 && is_null_free(o)) {
                *base = b;
                *offset = o;
                return 1;
            }
        }
    }

    return 0;
}

// ============================================================================
// Strategy: Arithmetic Constant Construction via SUB
// ============================================================================

int can_handle_arithmetic_constant_construction_sub(cs_insn *insn) {
    // Handle MOV reg, imm where imm contains nulls
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    // Must be register, immediate
    if (insn->detail->x86.operands[0].type != X86_OP_REG ||
        insn->detail->x86.operands[1].type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;

    // Only handle if immediate has null bytes
    if (is_null_free(imm)) {
        return 0;
    }

    // Check if we can construct via SUB
    uint32_t base, offset;
    return find_sub_construction(imm, &base, &offset);
}

size_t get_arithmetic_constant_construction_sub_size(cs_insn *insn) {
    (void)insn;
    // MOV reg, base (5-6 bytes) + SUB reg, offset (5-6 bytes) = ~10 bytes
    return 12;
}

void generate_arithmetic_constant_construction_sub(struct buffer *b, cs_insn *insn) {
    uint8_t dest_reg = insn->detail->x86.operands[0].reg;
    uint32_t target = (uint32_t)insn->detail->x86.operands[1].imm;

    uint32_t base, offset;
    if (!find_sub_construction(target, &base, &offset)) {
        // Fallback
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    uint8_t reg_idx = get_reg_index(dest_reg);

    // Check if 16-bit register (AX, BX, CX, DX)
    int is_16bit = (dest_reg == X86_REG_AX || dest_reg == X86_REG_BX ||
                    dest_reg == X86_REG_CX || dest_reg == X86_REG_DX);

    if (is_16bit) {
        // MOV reg16, base (66 B8+r + word)
        uint8_t mov_bytes[] = {0x66, (uint8_t)(0xB8 + reg_idx), 0x00, 0x00};
        mov_bytes[2] = base & 0xFF;
        mov_bytes[3] = (base >> 8) & 0xFF;
        buffer_append(b, mov_bytes, 4);

        // SUB reg16, offset (66 81 E8+r + word)
        uint8_t sub_bytes[] = {0x66, 0x81, (uint8_t)(0xE8 + reg_idx), 0x00, 0x00};
        sub_bytes[3] = offset & 0xFF;
        sub_bytes[4] = (offset >> 8) & 0xFF;
        buffer_append(b, sub_bytes, 5);
    } else {
        // MOV reg32, base
        uint8_t mov_bytes[] = {(uint8_t)(0xB8 + reg_idx), 0x00, 0x00, 0x00, 0x00};
        mov_bytes[1] = base & 0xFF;
        mov_bytes[2] = (base >> 8) & 0xFF;
        mov_bytes[3] = (base >> 16) & 0xFF;
        mov_bytes[4] = (base >> 24) & 0xFF;
        buffer_append(b, mov_bytes, 5);

        // SUB reg32, offset
        uint8_t sub_bytes[] = {0x81, (uint8_t)(0xE8 + reg_idx), 0x00, 0x00, 0x00, 0x00};
        sub_bytes[2] = offset & 0xFF;
        sub_bytes[3] = (offset >> 8) & 0xFF;
        sub_bytes[4] = (offset >> 16) & 0xFF;
        sub_bytes[5] = (offset >> 24) & 0xFF;
        buffer_append(b, sub_bytes, 6);
    }
}

strategy_t arithmetic_constant_construction_sub_strategy = {
    .name = "Arithmetic Constant Construction via SUB",
    .can_handle = can_handle_arithmetic_constant_construction_sub,
    .get_size = get_arithmetic_constant_construction_sub_size,
    .generate = generate_arithmetic_constant_construction_sub,
    .priority = 79
};

void register_arithmetic_constant_construction_sub_strategy() {
    register_strategy(&arithmetic_constant_construction_sub_strategy);
}
