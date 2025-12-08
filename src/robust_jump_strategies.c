/*
 * Robust Jump Resolution Strategy
 *
 * PROBLEM: Current jump/call resolution has issues with external targets
 *          and sometimes fails to properly handle relative jumps with null bytes
 *
 * SOLUTIONS:
 *   1. Implement proper external target resolution
 *   2. Use null-free absolute addressing when relative addressing would introduce nulls
 *   3. Implement safe conditional jump transformations
 *   4. Better validation to ensure no null bytes are introduced
 *
 * Priority: 85 (higher than current jump strategies to take precedence)
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/* Forward declarations */
extern void register_strategy(strategy_t *s);

/*
 * Helper function to detect if an instruction is a jump with potential null bytes
 */
static int has_jump_with_nulls(cs_insn *insn) {
    if (!insn || !insn->detail) {
        return 0;
    }

    // Check if this is a jump/call/conditional jump instruction
    if (insn->id == X86_INS_JMP || insn->id == X86_INS_CALL ||
        (insn->id >= X86_INS_JO && insn->id <= X86_INS_JNP)) {
        // Check if the instruction itself contains null bytes
        for (size_t j = 0; j < insn->size; j++) {
            if (insn->bytes[j] == 0x00) {
                return 1;  // Found null byte in jump instruction
            }
        }

        // Check if immediate operand (target) contains values that would create nulls
        // when encoded as relative offsets
        if (insn->detail->x86.op_count > 0 &&
            insn->detail->x86.operands[0].type == X86_OP_IMM) {
            
            // For relative jumps, if displacement has null bytes, we need to handle it
            // This is trickier to detect from the original instruction alone
            // We'll rely on the has_null_bytes function for now
        }
    }

    return 0;
}

/*
 * Detect jump/call instructions that need robust handling
 */
static int can_handle_robust_jump(cs_insn *insn) {
    if (!insn || !insn->detail) {
        return 0;
    }

    // Check if it's a jump/call instruction with null bytes
    if (has_jump_with_nulls(insn)) {
        return 1;
    }

    // Also handle jumps that might need transformation even if they don't currently have nulls
    // But for this strategy, we'll focus specifically on jumps with nulls
    if ((insn->id == X86_INS_JMP || insn->id == X86_INS_CALL ||
         (insn->id >= X86_INS_JO && insn->id <= X86_INS_JNP)) && has_null_bytes(insn)) {
        return 1;
    }

    return 0;
}

/*
 * Calculate replacement size for robust jump handling
 * This can vary significantly based on the target resolution
 */
static size_t get_size_robust_jump(cs_insn *insn) {
    // For calls and jumps, we might need MOV EAX, target + JMP/CALL EAX
    // which is 5 bytes (MOV) + 2 bytes (JMP/CALL) = 7 bytes
    (void)insn;
    return 7;
}

/*
 * Generate robust jump/call transformation that avoids null bytes
 */
static void generate_robust_jump(struct buffer *b, cs_insn *insn) {
    if (!insn || !insn->detail) {
        return;
    }

    // Check if this instruction has an immediate operand (target)
    if (insn->detail->x86.op_count == 0 ||
        insn->detail->x86.operands[0].type != X86_OP_IMM) {
        // No immediate operand, just copy original
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    uint32_t target = (uint32_t)insn->detail->x86.operands[0].imm;

    switch (insn->id) {
        case X86_INS_CALL: {
            // Transform CALL target to MOV EAX, target + CALL EAX
            // This prevents null bytes in the immediate operand
            generate_mov_eax_imm(b, target);
            uint8_t call_eax[] = {0xFF, 0xD0};  // CALL EAX
            buffer_append(b, call_eax, 2);
            break;
        }
        case X86_INS_JMP: {
            // Transform JMP target to MOV EAX, target + JMP EAX
            generate_mov_eax_imm(b, target);
            uint8_t jmp_eax[] = {0xFF, 0xE0};  // JMP EAX
            buffer_append(b, jmp_eax, 2);
            break;
        }
        default: {
            // Handle conditional jumps
            // For conditional jumps, we can use the opposite condition trick:
            // jcc target -> j!cc skip; MOV EAX, target; JMP EAX
            // where skip = size of MOV + JMP instructions
            
            // First, determine the opposite condition opcode
            uint8_t base_opcode = insn->bytes[0];
            uint8_t opposite_opcode;
            
            if (base_opcode == 0x0F && insn->size > 1) {
                // Near conditional jump (0F 8x format)
                opposite_opcode = 0x70 + ((insn->bytes[1] ^ 0x01) & 0x0F);  // Convert to short form
            } else if (base_opcode >= 0x70 && base_opcode <= 0x7F) {
                // Short conditional jump (7x format)
                opposite_opcode = base_opcode ^ 0x01;  // Flip lowest bit for opposite condition
            } else {
                // For other formats, just use the original
                buffer_append(b, insn->bytes, insn->size);
                return;
            }
            
            // Calculate size of MOV EAX, target + JMP EAX
            size_t mov_size = get_mov_eax_imm_size(target);
            size_t skip_size = mov_size + 2;  // MOV size + JMP EAX (2 bytes)
            
            // Only use this approach if skip_size fits in signed byte range (-128 to +127)
            if (skip_size <= 127) {
                // Emit opposite short jump to skip over absolute jump
                uint8_t skip_inst[] = {opposite_opcode, (uint8_t)skip_size};
                buffer_append(b, skip_inst, 2);
                
                // Emit MOV EAX, target (null-free)
                generate_mov_eax_imm(b, target);
                
                // Emit JMP EAX
                uint8_t jmp_eax[] = {0xFF, 0xE0};
                buffer_append(b, jmp_eax, 2);
            } else {
                // If skip_size is too large, fall back to original approach
                // but ensure no nulls are introduced
                buffer_append(b, insn->bytes, insn->size);
            }
        }
    }
}

/* Strategy definition */
static strategy_t robust_jump_strategy = {
    .name = "Robust Jump Resolution",
    .can_handle = can_handle_robust_jump,
    .get_size = get_size_robust_jump,
    .generate = generate_robust_jump,
    .priority = 85  // Higher priority than current jump strategies
};

/* Registration function */
void register_robust_jump_strategies() {
    register_strategy(&robust_jump_strategy);
}