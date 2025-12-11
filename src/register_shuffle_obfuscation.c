/*
 * BYVALVER - Register Shuffling Obfuscation (Priority 82)
 *
 * Uses XCHG operations to shuffle registers temporarily.
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

static int shuffle_counter = 0;
static int shuffle_initialized = 0;

// ============================================================================
// Helper: Generate XCHG instruction
// ============================================================================

static void generate_xchg_regs(struct buffer *b, uint8_t reg1_idx, uint8_t reg2_idx) {
    // XCHG reg1, reg2
    // Special case: XCHG EAX, reg uses short form (0x90 + reg_idx)
    if (reg1_idx == 0) {  // EAX
        uint8_t xchg_byte = 0x90 + reg2_idx;
        buffer_append(b, &xchg_byte, 1);
    } else if (reg2_idx == 0) {  // EAX
        uint8_t xchg_byte = 0x90 + reg1_idx;
        buffer_append(b, &xchg_byte, 1);
    } else {
        // General form: 87 ModR/M
        uint8_t xchg_bytes[] = {0x87, (uint8_t)(0xC0 | (reg1_idx << 3) | reg2_idx)};
        buffer_append(b, xchg_bytes, 2);
    }
}

// ============================================================================
// Strategy: Register Shuffle Obfuscation
// ============================================================================

int can_handle_register_shuffle(cs_insn *insn) {
    // Initialize once
    if (!shuffle_initialized) {
        srand((unsigned int)time(NULL) ^ 0x5678);
        shuffle_initialized = 1;
    }

    // Skip control flow instructions
    if (insn->id == X86_INS_JMP || insn->id == X86_INS_CALL ||
        insn->id == X86_INS_RET || insn->id == X86_INS_RETF) {
        return 0;
    }

    // Skip conditional jumps
    if (insn->id >= X86_INS_JAE && insn->id <= X86_INS_JS) {
        return 0;
    }

    // Skip PUSH/POP to avoid stack corruption
    if (insn->id == X86_INS_PUSH || insn->id == X86_INS_POP) {
        return 0;
    }

    // Apply to approximately 12% of instructions
    shuffle_counter++;
    return (shuffle_counter % 8 == 0);
}

size_t get_register_shuffle_size(cs_insn *insn) {
    // Original instruction + 2 XCHG operations (canceling)
    // XCHG can be 1-2 bytes each
    return insn->size + 4;
}

void generate_register_shuffle(struct buffer *b, cs_insn *insn) {
    // Insert self-canceling XCHG operations
    // Pattern: XCHG reg1, reg2; XCHG reg1, reg2 (double XCHG = NOP)

    // Select two registers to shuffle (avoid ESP/EBP)
    // Use ESI (6) and EDI (7) for simplicity
    uint8_t reg1_idx = 6;  // ESI
    uint8_t reg2_idx = 7;  // EDI

    // Randomly pick register pair
    int pair = rand() % 3;
    switch (pair) {
        case 0:
            reg1_idx = 1;  // ECX
            reg2_idx = 2;  // EDX
            break;
        case 1:
            reg1_idx = 3;  // EBX
            reg2_idx = 6;  // ESI
            break;
        case 2:
            reg1_idx = 6;  // ESI
            reg2_idx = 7;  // EDI
            break;
    }

    // First XCHG
    generate_xchg_regs(b, reg1_idx, reg2_idx);

    // Second XCHG (restores original state)
    generate_xchg_regs(b, reg1_idx, reg2_idx);

    // Insert original instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t register_shuffle_strategy = {
    .name = "Register Shuffle Obfuscation",
    .can_handle = can_handle_register_shuffle,
    .get_size = get_register_shuffle_size,
    .generate = generate_register_shuffle,
    .priority = 82
};

void register_register_shuffle_obfuscation() {
    register_obfuscation_strategy(&register_shuffle_strategy);
}
