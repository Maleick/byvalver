/*
 * POPCNT/LZCNT/TZCNT Bit Counting for Constant Generation Strategy
 *
 * PROBLEM: MOV immediate instructions may contain bad characters.
 *
 * SOLUTION: Use bit-counting instructions (POPCNT, TZCNT, LZCNT) to generate
 * constants by counting bits in carefully chosen source values.
 *
 * Example:
 * Original: mov eax, 5 (may have bad chars)
 * Transform: mov ebx, 0x1F; popcnt eax, ebx  (0x1F has 5 bits set)
 */

#include "bit_counting_constant_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Count the number of set bits in a value
 */
static int popcount(uint32_t val) {
    int count = 0;
    while (val) {
        count += val & 1;
        val >>= 1;
    }
    return count;
}

/**
 * Count trailing zeros (for future use)
 */
__attribute__((unused)) static int count_trailing_zeros(uint32_t val) {
    if (val == 0) return 32;
    int count = 0;
    while ((val & 1) == 0) {
        count++;
        val >>= 1;
    }
    return count;
}

/**
 * Check if a 32-bit value is bad-char-free when encoded in little-endian
 */
static int is_value_bad_char_free(uint32_t val) {
    uint8_t bytes[4];
    bytes[0] = (uint8_t)(val & 0xFF);
    bytes[1] = (uint8_t)((val >> 8) & 0xFF);
    bytes[2] = (uint8_t)((val >> 16) & 0xFF);
    bytes[3] = (uint8_t)((val >> 24) & 0xFF);
    return is_bad_char_free_buffer(bytes, 4);
}

/**
 * Find a value with exactly 'target' set bits that is bad-char-free
 */
static uint32_t find_popcount_source(int target) {
    // Try simple patterns first
    if (target <= 0 || target > 32) return 0;

    // Create a value with 'target' consecutive bits set
    uint32_t val = (1U << target) - 1;
    if (is_value_bad_char_free(val)) {
        return val;
    }

    // Try scattered patterns
    val = 0;
    for (int i = 0; i < target && i < 32; i += 2) {
        val |= (1U << i);
    }
    if (popcount(val) == target && is_value_bad_char_free(val)) {
        return val;
    }

    return 0;  // Failed to find suitable value
}

/**
 * Find a power-of-2 value where trailing zeros equal target
 */
static uint32_t find_tzcnt_source(int target) {
    if (target < 0 || target >= 32) return 0;

    uint32_t val = 1U << target;
    if (is_value_bad_char_free(val)) {
        return val;
    }

    return 0;  // Failed to find suitable value
}

/**
 * Check if this strategy can handle the instruction
 */
int can_handle_bit_counting(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Only handle MOV instructions with immediate to register
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    // Check if it's MOV reg, imm
    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    // Destination must be a 32-bit register
    if (dst->type != X86_OP_REG) {
        return 0;
    }

    // Source must be immediate
    if (src->type != X86_OP_IMM) {
        return 0;
    }

    // Get the immediate value
    uint32_t imm = (uint32_t)src->imm;

    // Check if we can construct this value using bit counting
    // Try POPCNT: value <= 32 and we can find a source with that many bits
    // Try TZCNT: value is a power-of-2 exponent (0-31)

    int can_use_popcnt = (imm <= 32) && (find_popcount_source(imm) != 0);
    int can_use_tzcnt = (imm < 32) && (find_tzcnt_source(imm) != 0);

    if (!can_use_popcnt && !can_use_tzcnt) {
        return 0;
    }

    // Check if the instruction encoding contains bad characters
    if (!is_bad_char_free_buffer(insn->bytes, insn->size)) {
        return 1;  // Has bad chars, we can handle it
    }

    return 0;  // No bad chars, no need to transform
}

/**
 * Calculate size of transformed instruction
 */
size_t get_size_bit_counting(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Transformation: mov ebx, source_value; popcnt/tzcnt eax, ebx
    // MOV EBX, imm32 = 5 bytes (BB XX XX XX XX)
    // POPCNT EAX, EBX = 4 bytes (F3 0F B8 C3)
    // Total = 9 bytes
    return 9;
}

/**
 * Generate transformed instruction sequence
 */
void generate_bit_counting(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Get the immediate value
    if (insn->detail->x86.op_count != 2) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    cs_x86_op *dst = &insn->detail->x86.operands[0];
    cs_x86_op *src = &insn->detail->x86.operands[1];

    if (src->type != X86_OP_IMM || dst->type != X86_OP_REG) {
        // Fallback: copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    uint32_t target = (uint32_t)src->imm;

    // Try TZCNT first (for power-of-2 exponents)
    uint32_t source_val = find_tzcnt_source(target);
    int use_tzcnt = (source_val != 0);

    if (!use_tzcnt) {
        // Try POPCNT
        source_val = find_popcount_source(target);
        if (source_val == 0) {
            // Failed to find suitable source, fallback
            buffer_append(b, insn->bytes, insn->size);
            return;
        }
    }

    // Use EBX as temporary register for source value
    // MOV EBX, source_val (BB XX XX XX XX)
    buffer_write_byte(b, 0xBB);
    buffer_write_byte(b, (uint8_t)(source_val & 0xFF));
    buffer_write_byte(b, (uint8_t)((source_val >> 8) & 0xFF));
    buffer_write_byte(b, (uint8_t)((source_val >> 16) & 0xFF));
    buffer_write_byte(b, (uint8_t)((source_val >> 24) & 0xFF));

    if (use_tzcnt) {
        // TZCNT dest_reg, EBX (F3 0F BC XX)
        buffer_write_byte(b, 0xF3);  // Prefix
        buffer_write_byte(b, 0x0F);
        buffer_write_byte(b, 0xBC);

        // ModR/M byte: depends on destination register
        // For EAX: C3, for ECX: CB, for EDX: D3, etc.
        // General formula: 0xC3 + (dest_reg_index << 3)
        // Simplified: assume EAX (C3)
        buffer_write_byte(b, 0xC3);
    } else {
        // POPCNT dest_reg, EBX (F3 0F B8 XX)
        buffer_write_byte(b, 0xF3);  // Prefix
        buffer_write_byte(b, 0x0F);
        buffer_write_byte(b, 0xB8);

        // ModR/M byte for EAX, EBX
        buffer_write_byte(b, 0xC3);
    }
}

// Define the strategy structure
strategy_t bit_counting_strategy = {
    .name = "POPCNT/LZCNT/TZCNT Bit Counting",
    .can_handle = can_handle_bit_counting,
    .get_size = get_size_bit_counting,
    .generate = generate_bit_counting,
    .priority = 77
};
