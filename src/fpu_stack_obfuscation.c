/*
 * BYVALVER - FPU Stack Obfuscation (Priority 86)
 *
 * Uses FPU instructions for obfuscation.
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

static int fpu_counter = 0;
static int fpu_initialized = 0;

// ============================================================================
// Strategy: Insert FPU NOPs and dead FPU operations
// ============================================================================

int can_handle_fpu_obfuscation(cs_insn *insn) {
    // Initialize once
    if (!fpu_initialized) {
        srand((unsigned int)time(NULL) ^ 0x1234);
        fpu_initialized = 1;
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

    // Skip existing FPU instructions
    if (insn->id >= X86_INS_FABS && insn->id <= X86_INS_FYL2XP1) {
        return 0;
    }

    // Apply to approximately 10% of instructions
    fpu_counter++;
    return (fpu_counter % 10 == 0);
}

size_t get_fpu_obfuscation_size(cs_insn *insn) {
    // Original instruction + FPU junk (varies)
    return insn->size + 6;
}

void generate_fpu_obfuscation(struct buffer *b, cs_insn *insn) {
    // Insert FPU junk before instruction
    int pattern = rand() % 4;

    switch (pattern) {
        case 0:
            // FNOP (FPU NOP)
            {
                uint8_t fnop[] = {0xD9, 0xD0};
                buffer_append(b, fnop, 2);
            }
            break;

        case 1:
            // FLD1; FSTP ST(0) (push 1.0, then pop it - no net effect)
            {
                uint8_t fld1[] = {0xD9, 0xE8};         // FLD1
                buffer_append(b, fld1, 2);
                uint8_t fstp[] = {0xDD, 0xD8};         // FSTP ST(0)
                buffer_append(b, fstp, 2);
            }
            break;

        case 2:
            // FLDZ; FSTP ST(0) (push 0.0, then pop it)
            {
                uint8_t fldz[] = {0xD9, 0xEE};         // FLDZ
                buffer_append(b, fldz, 2);
                uint8_t fstp[] = {0xDD, 0xD8};         // FSTP ST(0)
                buffer_append(b, fstp, 2);
            }
            break;

        case 3:
            // FLDPI; FSTP ST(0) (push PI, then pop it)
            {
                uint8_t fldpi[] = {0xD9, 0xEB};        // FLDPI
                buffer_append(b, fldpi, 2);
                uint8_t fstp[] = {0xDD, 0xD8};         // FSTP ST(0)
                buffer_append(b, fstp, 2);
            }
            break;
    }

    // Pad to 6 bytes total FPU junk with FNOPs if needed
    if (pattern == 0) {
        uint8_t fnop[] = {0xD9, 0xD0};
        buffer_append(b, fnop, 2);
        buffer_append(b, fnop, 2);
    }

    // Insert original instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t fpu_obfuscation_strategy = {
    .name = "FPU Stack Obfuscation",
    .can_handle = can_handle_fpu_obfuscation,
    .get_size = get_fpu_obfuscation_size,
    .generate = generate_fpu_obfuscation,
    .priority = 86
};

void register_fpu_stack_obfuscation() {
    register_obfuscation_strategy(&fpu_obfuscation_strategy);
}
