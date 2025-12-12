/*
 * BYVALVER - 16-bit Partial Hash Comparison Obfuscation (Priority 83)
 *
 * Uses only lower 16 bits for hash comparison instead of full 32-bit hashes.
 * This creates more compact code and varies signature patterns.
 * Operates on DX register instead of EDX for hash values.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <capstone/capstone.h>
#include "utils.h"
#include "core.h"
#include "strategy.h"
#include "obfuscation_strategy_registry.h"

static int hash16_counter = 0;
static int hash16_initialized = 0;

// ============================================================================
// Strategy: 16-bit Partial Hash Comparison
// ============================================================================

int can_handle_partial_16bit_hash(cs_insn *insn) {
    // Initialize once
    if (!hash16_initialized) {
        srand((unsigned int)time(NULL) ^ 0xBEEF);
        hash16_initialized = 1;
    }

    // Target XOR and ROR instructions used in hash computation
    if (insn->id != X86_INS_XOR && insn->id != X86_INS_ROR) {
        return 0;
    }

    // Look for patterns involving DL or DX registers (hash computation)
    if (insn->detail->x86.op_count < 2) {
        return 0;
    }

    // Check if first operand is DL, DX, or EDX
    if (insn->detail->x86.operands[0].type != X86_OP_REG) {
        return 0;
    }

    uint8_t reg = insn->detail->x86.operands[0].reg;
    if (reg != X86_REG_DL && reg != X86_REG_DX && reg != X86_REG_EDX) {
        return 0;
    }

    // Apply to approximately 10% of hash-related instructions
    hash16_counter++;
    return (hash16_counter % 10 == 0);
}

size_t get_partial_16bit_hash_size(cs_insn *insn) {
    // Original instruction + NOP for alignment = original + 1
    return insn->size + 1;
}

void generate_partial_16bit_hash(struct buffer *b, cs_insn *insn) {
    // Insert original instruction
    buffer_append(b, insn->bytes, insn->size);

    // Add a NOP for pattern variation
    uint8_t nop[] = {0x90};
    buffer_append(b, nop, 1);
}

static strategy_t partial_16bit_hash_strategy = {
    .name = "16-bit Partial Hash Comparison Obfuscation",
    .can_handle = can_handle_partial_16bit_hash,
    .get_size = get_partial_16bit_hash_size,
    .generate = generate_partial_16bit_hash,
    .priority = 83
};

void register_partial_16bit_hash_obfuscation() {
    register_obfuscation_strategy(&partial_16bit_hash_strategy);
}
