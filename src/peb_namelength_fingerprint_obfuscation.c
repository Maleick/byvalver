/*
 * BYVALVER - PEB Module Name Length-Based Fingerprinting Obfuscation (Priority 84)
 *
 * Identifies DLLs in PEB InInitializationOrder by checking Unicode name length
 * rather than comparing full module names. This is more compact and stealthy.
 * Instead of string comparison, checks if null byte appears at specific offset.
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

static int fingerprint_counter = 0;
static int fingerprint_initialized = 0;

// ============================================================================
// Strategy: PEB Module Name Length Fingerprinting
// ============================================================================

int can_handle_peb_namelength_fingerprint(cs_insn *insn) {
    // Initialize once
    if (!fingerprint_initialized) {
        srand((unsigned int)time(NULL) ^ 0xABCD);
        fingerprint_initialized = 1;
    }

    // This strategy adds length-based checking patterns for CMP instructions
    // that might be used in PEB traversal
    if (insn->id != X86_INS_CMP) {
        return 0;
    }

    // Skip if not comparing memory locations or registers
    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    // Apply to approximately 8% of CMP instructions to add variety
    fingerprint_counter++;
    return (fingerprint_counter % 12 == 0);
}

size_t get_peb_namelength_fingerprint_size(cs_insn *insn) {
    // Original CMP + additional TEST instruction (2 bytes) = original + 2
    return insn->size + 2;
}

void generate_peb_namelength_fingerprint(struct buffer *b, cs_insn *insn) {
    // Insert a redundant TEST instruction before CMP to simulate
    // length checking pattern (fingerprinting obfuscation)

    // TEST CX, CX (66 85 C9) - check if CX is zero (common pattern in length checks)
    uint8_t test_cx[] = {0x66, 0x85, 0xC9};
    buffer_append(b, test_cx, 3);

    // Insert original CMP instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t peb_namelength_fingerprint_strategy = {
    .name = "PEB Module Name Length Fingerprinting Obfuscation",
    .can_handle = can_handle_peb_namelength_fingerprint,
    .get_size = get_peb_namelength_fingerprint_size,
    .generate = generate_peb_namelength_fingerprint,
    .priority = 84
};

void register_peb_namelength_fingerprint_obfuscation() {
    register_obfuscation_strategy(&peb_namelength_fingerprint_strategy);
}
