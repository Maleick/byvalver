/*
 * BYVALVER - Mutated Junk Insertion Obfuscation (Priority 93)
 *
 * Creates complex control flow with opaque predicates and dead code paths.
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

// Static counter for applying junk insertion selectively
static int junk_counter = 0;
static int initialized = 0;

// ============================================================================
// Opaque Predicate Patterns
// ============================================================================

// Pattern 1: XOR reg, reg; JZ +N (always taken)
static void insert_always_taken_jz(struct buffer *b, uint8_t junk_size) {
    // XOR EBX, EBX
    uint8_t xor_bytes[] = {0x31, 0xDB};
    buffer_append(b, xor_bytes, 2);

    // JZ +junk_size (always taken since EBX is zero)
    uint8_t jz_bytes[] = {0x74, junk_size};
    buffer_append(b, jz_bytes, 2);
}

// Pattern 2: TEST EAX, EAX; JNZ +N after XOR (never taken)
static void insert_never_taken_jnz(struct buffer *b, uint8_t junk_size) {
    // XOR EAX, EAX
    uint8_t xor_bytes[] = {0x31, 0xC0};
    buffer_append(b, xor_bytes, 2);

    // JNZ +junk_size (never taken since EAX is zero)
    uint8_t jnz_bytes[] = {0x75, junk_size};
    buffer_append(b, jnz_bytes, 2);
}

// Pattern 3: CMP reg, reg; JE +N (always taken)
static void insert_always_taken_je(struct buffer *b, uint8_t junk_size) {
    // CMP ECX, ECX
    uint8_t cmp_bytes[] = {0x39, 0xC9};
    buffer_append(b, cmp_bytes, 2);

    // JE +junk_size (always taken since ECX == ECX)
    uint8_t je_bytes[] = {0x74, junk_size};
    buffer_append(b, je_bytes, 2);
}

// ============================================================================
// Junk Code Payloads
// ============================================================================

// Junk payload 1: INT3 instructions (would crash if executed)
static void insert_int3_junk(struct buffer *b, int count) {
    for (int i = 0; i < count; i++) {
        uint8_t int3 = 0xCC;
        buffer_append(b, &int3, 1);
    }
}

// Junk payload 2: Invalid multi-byte NOPs
static void insert_garbage_bytes(struct buffer *b, int count) {
    uint8_t garbage[] = {0x90, 0x90, 0xCC, 0xCC, 0xF1, 0xF4};
    for (int i = 0; i < count && i < 6; i++) {
        buffer_append(b, &garbage[i], 1);
    }
}

// Junk payload 3: Dead register manipulations
static void insert_dead_operations(struct buffer *b) {
    // PUSH EDI; POP EDI (no effect)
    uint8_t bytes1[] = {0x57, 0x5F};
    buffer_append(b, bytes1, 2);

    // XOR ESI, ESI; XOR ESI, ESI (restore to 0)
    uint8_t bytes2[] = {0x31, 0xF6, 0x31, 0xF6};
    buffer_append(b, bytes2, 4);
}

// ============================================================================
// Main Strategy: Mutated Junk Insertion
// ============================================================================

int can_handle_mutated_junk(cs_insn *insn) {
    // Initialize random seed once
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }

    // Skip jumps and calls to avoid breaking control flow
    if (insn->id == X86_INS_JMP || insn->id == X86_INS_CALL ||
        insn->id == X86_INS_RET || insn->id == X86_INS_RETF) {
        return 0;
    }

    // Skip conditional jumps
    if (insn->id >= X86_INS_JAE && insn->id <= X86_INS_JS) {
        return 0;
    }

    // Apply to approximately 15% of instructions
    junk_counter++;
    return (junk_counter % 7 == 0);
}

size_t get_mutated_junk_size(cs_insn *insn) {
    // Original instruction + opaque predicate (4 bytes) + junk (6 bytes)
    return insn->size + 10;
}

void generate_mutated_junk(struct buffer *b, cs_insn *insn) {
    // Randomly select an opaque predicate pattern
    int pattern = rand() % 3;
    uint8_t junk_size = 6;  // Size of junk code block

    // Insert opaque predicate that jumps over junk code
    switch (pattern) {
        case 0:
            insert_always_taken_jz(b, junk_size);
            break;
        case 1:
            insert_always_taken_je(b, junk_size);
            break;
        case 2:
            insert_never_taken_jnz(b, junk_size);
            // For never-taken, we insert junk that WILL be skipped
            break;
    }

    // Insert junk code block (unreachable if opaque predicate is correct)
    if (pattern == 2) {
        // Never-taken path: insert dangerous junk
        insert_int3_junk(b, 6);
    } else {
        // Always-taken path: insert harmless junk that will be skipped
        int junk_type = rand() % 3;
        switch (junk_type) {
            case 0:
                insert_int3_junk(b, 6);
                break;
            case 1:
                insert_garbage_bytes(b, 6);
                break;
            case 2:
                insert_dead_operations(b);
                break;
        }
    }

    // Finally, insert the original instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t mutated_junk_strategy = {
    .name = "Mutated Junk Insertion Obfuscation",
    .can_handle = can_handle_mutated_junk,
    .get_size = get_mutated_junk_size,
    .generate = generate_mutated_junk,
    .priority = 93
};

void register_mutated_junk_insertion_obfuscation() {
    register_obfuscation_strategy(&mutated_junk_strategy);
}
