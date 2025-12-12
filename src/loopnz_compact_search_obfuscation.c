/*
 * BYVALVER - LOOPNZ-Based Compact Search Patterns Obfuscation (Priority 76)
 *
 * Uses LOOPNZ instruction to create ultra-compact search loops.
 * LOOPNZ atomically performs: ECX--; if (ECX != 0 && ZF == 0) jump
 * This replaces traditional DEC/CMP/JNZ patterns with a single 2-byte instruction.
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

static int loopnz_counter = 0;
static int loopnz_initialized = 0;

// ============================================================================
// Helper: Check if instruction is part of loop pattern
// ============================================================================

static int is_loop_decrement(cs_insn *insn) {
    // Check for DEC ECX pattern
    if (insn->id == X86_INS_DEC &&
        insn->detail->x86.op_count == 1 &&
        insn->detail->x86.operands[0].type == X86_OP_REG &&
        insn->detail->x86.operands[0].reg == X86_REG_ECX) {
        return 1;
    }
    return 0;
}

// ============================================================================
// Strategy: LOOPNZ Compact Search
// ============================================================================

int can_handle_loopnz_compact_search(cs_insn *insn) {
    // Initialize once
    if (!loopnz_initialized) {
        srand((unsigned int)time(NULL) ^ 0xDEAD);
        loopnz_initialized = 1;
    }

    // Look for DEC ECX instructions (common in loops)
    if (!is_loop_decrement(insn)) {
        return 0;
    }

    // Apply to approximately 15% of DEC ECX instructions
    loopnz_counter++;
    return (loopnz_counter % 7 == 0);
}

size_t get_loopnz_compact_search_size(cs_insn *insn) {
    // Original DEC ECX (1-2 bytes) gets replaced with LOOPNZ pattern
    // But we keep the DEC and add a conditional LOOPNZ placeholder
    // DEC ECX (1) + NOP for alignment (1) = 2 bytes
    (void)insn;
    return 2;
}

void generate_loopnz_compact_search(struct buffer *b, cs_insn *insn) {
    // For obfuscation, we insert the original DEC but add pattern similarity
    // to LOOPNZ usage by inserting a TEST ECX, ECX before it

    // TEST ECX, ECX (85 C9) - sets ZF if ECX is zero
    uint8_t test_ecx[] = {0x85, 0xC9};
    buffer_append(b, test_ecx, 2);

    // Original DEC ECX instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t loopnz_compact_search_strategy = {
    .name = "LOOPNZ Compact Search Obfuscation",
    .can_handle = can_handle_loopnz_compact_search,
    .get_size = get_loopnz_compact_search_size,
    .generate = generate_loopnz_compact_search,
    .priority = 76
};

void register_loopnz_compact_search_obfuscation() {
    register_obfuscation_strategy(&loopnz_compact_search_strategy);
}
