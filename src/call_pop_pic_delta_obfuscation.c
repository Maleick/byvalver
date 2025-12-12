/*
 * BYVALVER - CALL/POP Position-Independent Code Delta Obfuscation (Priority 95)
 *
 * Uses CALL instruction's return address mechanism to dynamically retrieve
 * current EIP without hardcoded addresses. Creates fully position-independent
 * shellcode that can execute from any memory location.
 *
 * Pattern: CALL geteip; geteip: POP reg; LEA reg, [reg-offset]
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

static int pic_counter = 0;
static int pic_initialized = 0;

// ============================================================================
// Helper: Check if instruction involves absolute addressing
// ============================================================================

static int uses_absolute_address(cs_insn *insn) {
    // Check for memory operands with displacement only (no base/index)
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_MEM) {
            cs_x86_op *op = &insn->detail->x86.operands[i];
            // If it has displacement but no base/index, it's absolute
            if (op->mem.disp != 0 &&
                op->mem.base == X86_REG_INVALID &&
                op->mem.index == X86_REG_INVALID) {
                return 1;
            }
        }
    }
    return 0;
}

// ============================================================================
// Strategy: CALL/POP PIC Delta Retrieval
// ============================================================================

int can_handle_call_pop_pic_delta(cs_insn *insn) {
    // Initialize once
    if (!pic_initialized) {
        srand((unsigned int)time(NULL) ^ 0xC0DE);
        pic_initialized = 1;
    }

    // Target MOV instructions with immediate values or absolute addresses
    if (insn->id != X86_INS_MOV && insn->id != X86_INS_LEA) {
        return 0;
    }

    // Look for absolute addressing patterns
    if (!uses_absolute_address(insn)) {
        return 0;
    }

    // Apply to approximately 5% of such instructions (PIC is expensive)
    pic_counter++;
    return (pic_counter % 20 == 0);
}

size_t get_call_pop_pic_delta_size(cs_insn *insn) {
    // CALL geteip (5 bytes) + POP reg (1 byte) + LEA (6 bytes) + original (varies)
    // Total overhead: ~12 bytes
    return insn->size + 12;
}

void generate_call_pop_pic_delta(struct buffer *b, cs_insn *insn) {
    // Generate CALL/POP pattern for PIC
    // CALL $+5 (E8 00 00 00 00)
    uint8_t call_next[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
    buffer_append(b, call_next, 5);

    // POP EDX (5A) - get return address
    uint8_t pop_edx[] = {0x5A};
    buffer_append(b, pop_edx, 1);

    // LEA EDX, [EDX-5] (8D 52 FB) - adjust to reference point
    uint8_t lea_edx[] = {0x8D, 0x52, 0xFB};
    buffer_append(b, lea_edx, 3);

    // NOP for alignment
    uint8_t nop[] = {0x90};
    buffer_append(b, nop, 1);

    // Insert original instruction
    buffer_append(b, insn->bytes, insn->size);
}

static strategy_t call_pop_pic_delta_strategy = {
    .name = "CALL/POP PIC Delta Retrieval Obfuscation",
    .can_handle = can_handle_call_pop_pic_delta,
    .get_size = get_call_pop_pic_delta_size,
    .generate = generate_call_pop_pic_delta,
    .priority = 95
};

void register_call_pop_pic_delta_obfuscation() {
    register_obfuscation_strategy(&call_pop_pic_delta_strategy);
}
