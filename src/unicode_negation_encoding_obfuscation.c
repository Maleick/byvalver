/*
 * BYVALVER - Unicode Negation Encoding Obfuscation (Priority 81)
 *
 * Constructs UTF-16/Unicode string values by using NEG instruction on carefully
 * crafted negative immediates, rather than directly PUSH-ing Unicode values.
 * Eliminates null bytes from Unicode string construction.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"
#include "obfuscation_strategy_registry.h"

// ============================================================================
// Helper: Check if value represents Unicode (has null bytes)
// ============================================================================

static int is_unicode_value(uint32_t val) {
    // Unicode values typically have pattern 0x00XX00YY or contain null bytes
    // Check if value has null bytes which suggest Unicode encoding
    if ((val & 0xFF) == 0 || ((val >> 8) & 0xFF) == 0 ||
        ((val >> 16) & 0xFF) == 0 || ((val >> 24) & 0xFF) == 0) {
        return 1;
    }
    return 0;
}

// ============================================================================
// Helper: Calculate NEG equivalent for Unicode value
// ============================================================================

static int find_neg_unicode_equivalent(uint32_t target, uint32_t *negated_val) {
    // NEG operation: result = 0 - operand (two's complement)
    // So: target = NEG(negated_val) means negated_val = -target
    // In two's complement: -target = ~target + 1

    uint32_t neg_val = (~target) + 1;

    // Check if the negated value is null-free
    if (is_null_free(neg_val)) {
        *negated_val = neg_val;
        return 1;
    }

    return 0;
}

// ============================================================================
// Strategy: Unicode Negation Encoding Obfuscation
// ============================================================================

int can_handle_unicode_negation_encoding(cs_insn *insn) {
    // Only handle PUSH instructions
    if (insn->id != X86_INS_PUSH) {
        return 0;
    }

    // Only handle immediate operands
    if (insn->detail->x86.op_count != 1 ||
        insn->detail->x86.operands[0].type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)insn->detail->x86.operands[0].imm;

    // Check if this looks like a Unicode value (has null bytes)
    if (!is_unicode_value(imm)) {
        return 0;
    }

    // Check if we can find a null-free NEG equivalent
    uint32_t negated_val;
    if (!find_neg_unicode_equivalent(imm, &negated_val)) {
        return 0;
    }

    return 1;
}

size_t get_unicode_negation_encoding_size(cs_insn *insn) {
    (void)insn;
    // MOV EDX, imm32 (5 bytes) + NEG EDX (2 bytes) + PUSH EDX (1 byte) = 8 bytes
    return 8;
}

void generate_unicode_negation_encoding(struct buffer *b, cs_insn *insn) {
    uint32_t target = (uint32_t)insn->detail->x86.operands[0].imm;
    uint32_t negated_val;

    // Calculate the negated value
    if (!find_neg_unicode_equivalent(target, &negated_val)) {
        // Fallback: just copy original instruction
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // MOV EDX, negated_val (0xBA + 4-byte immediate)
    uint8_t mov_edx[] = {0xBA, 0x00, 0x00, 0x00, 0x00};
    mov_edx[1] = negated_val & 0xFF;
    mov_edx[2] = (negated_val >> 8) & 0xFF;
    mov_edx[3] = (negated_val >> 16) & 0xFF;
    mov_edx[4] = (negated_val >> 24) & 0xFF;
    buffer_append(b, mov_edx, 5);

    // NEG EDX (0xF7 0xDA)
    uint8_t neg_edx[] = {0xF7, 0xDA};
    buffer_append(b, neg_edx, 2);

    // PUSH EDX (0x52)
    uint8_t push_edx[] = {0x52};
    buffer_append(b, push_edx, 1);
}

static strategy_t unicode_negation_encoding_strategy = {
    .name = "Unicode Negation Encoding Obfuscation",
    .can_handle = can_handle_unicode_negation_encoding,
    .get_size = get_unicode_negation_encoding_size,
    .generate = generate_unicode_negation_encoding,
    .priority = 81
};

void register_unicode_negation_encoding_obfuscation() {
    register_obfuscation_strategy(&unicode_negation_encoding_strategy);
}
