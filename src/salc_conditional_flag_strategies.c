/*
 * SALC + Conditional Flag Manipulation Strategy
 *
 * PROBLEM: Setting AL register to 0 or other values that might contain nulls
 * in MOV instructions. Also manipulating flags without operations that contain nulls.
 * Additionally, modern shellcode uses SALC combined with REP STOSB for efficient
 * null-filled buffer initialization, which requires advanced handling.
 *
 * SOLUTION: Use SALC (Set AL on Carry) instruction combined with flag manipulation
 * to set AL register efficiently, and provide advanced patterns for SALC+REP STOSB usage.
 *
 * FREQUENCY: Common in 32-bit shellcode for efficient zeroing without nulls;
 *            SALC+REP STOSB increasingly common for buffer initialization
 * PRIORITY: 93 (Very High - advanced efficient approach)
 *
 * Example transformations:
 *   Original: MOV AL, 0x00 (B0 00 - contains null)
 *   Strategy: CLC; SALC (F8 D6 - no nulls)
 *   Advanced: SALC + REP STOSB for efficient null buffer filling
 */

#include "salc_conditional_flag_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV AL, imm8 where imm8 is 0 or other patterns
 * Extended to also detect patterns relevant to SALC+REP STOSB usage
 */
int can_handle_salc_conditional_flag(cs_insn *insn) {
    if (!insn || !insn->detail) return 0;

    // Handle original SALC case: MOV AL, 0x00 or MOV AL, 0xFF
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // Must be MOV register, immediate
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // Must be MOV AL specifically
            if (dst_op->reg == X86_REG_AL) {
                uint8_t imm = (uint8_t)src_op->imm;

                // SALC can set AL to 0x00 (CF=0) or 0xFF (CF=1)
                if (imm == 0x00 || imm == 0xFF) {
                    return 1;
                }
            }
        }
    }

    // Extended detection for instructions that might be part of SALC+REP STOSB sequences
    // Look for REP STOSB instructions directly
    if (insn->id == X86_INS_STOSB && (insn->detail->x86.prefix[0] == 0xF3 || insn->detail->x86.prefix[1] == 0xF3)) {
        // This is a REP STOSB instruction, possibly following SALC
        return 1;
    }

    // Look for MOV ECX, immediate patterns that might be for REP STOSB count
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_REG && dst_op->reg == X86_REG_ECX && src_op->type == X86_OP_IMM) {
            // This is MOV ECX, imm - potentially for REP STOSB count
            // Check if immediate contains nulls
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    // Look for MOV EDI, immediate patterns that might be for REP STOSB destination
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_REG && dst_op->reg == X86_REG_EDI && src_op->type == X86_OP_IMM) {
            // This is MOV EDI, imm - potentially for REP STOSB destination address
            // Check if immediate contains nulls
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Size calculation for SALC + flag manipulation and advanced patterns
 */
size_t get_size_salc_conditional_flag(cs_insn *insn) {
    if (!insn || !insn->detail) return 2;

    // For REP STOSB, we might need additional handling
    if (insn->id == X86_INS_STOSB && (insn->detail->x86.prefix[0] == 0xF3 || insn->detail->x86.prefix[1] == 0xF3)) {
        // This is REP STOSB, part of a sequence, so size might vary based on register setup
        return 2; // Just the instruction itself
    }

    // For MOV ECX/EDI with immediate values that contain nulls
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if ((dst_op->type == X86_OP_REG && (dst_op->reg == X86_REG_ECX || dst_op->reg == X86_REG_EDI)) &&
            src_op->type == X86_OP_IMM) {
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                // Would require null-safe approach: MOV EAX, imm; MOV ECX/EDI, EAX
                return 6; // Estimate for MOV EAX, imm32 + MOV reg, EAX
            }
        }
    }

    // Default for original SALC case
    // CLC (1) + SALC (1) = 2 bytes for zeroing AL
    // STC (1) + SALC (1) = 2 bytes for setting AL to 0xFF
    return 2;
}

/*
 * Generate SALC + flag manipulation sequence with advanced patterns
 */
void generate_salc_conditional_flag(struct buffer *b, cs_insn *insn) {
    if (!insn || !insn->detail) return;

    // Store the initial size to verify no nulls are introduced
    size_t initial_size = b->size;

    // Handle original SALC case: MOV AL, imm8 where imm is 0 or 0xFF
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_REG && dst_op->reg == X86_REG_AL && src_op->type == X86_OP_IMM) {
            uint8_t target_value = (uint8_t)src_op->imm;

            if (target_value == 0x00) {
                // CLC - Clear carry flag
                buffer_write_byte(b, 0xF8);
                // SALC - Set AL based on carry (AL = 0x00 when CF=0)
                buffer_write_byte(b, 0xD6);
            } else if (target_value == 0xFF) {
                // STC - Set carry flag
                buffer_write_byte(b, 0xF9);
                // SALC - Set AL based on carry (AL = 0xFF when CF=1)
                buffer_write_byte(b, 0xD6);
            }
            return;
        }

        // Handle MOV ECX, imm or MOV EDI, imm that might contain nulls
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            if (dst_op->reg == X86_REG_ECX || dst_op->reg == X86_REG_EDI) {
                uint32_t imm = (uint32_t)src_op->imm;

                if (!is_bad_char_free(imm)) {
                    // Use null-safe approach: MOV EAX, imm; MOV target_reg, EAX
                    generate_mov_eax_imm(b, imm);

                    // Now MOV target_reg, EAX
                    uint8_t target_reg = dst_op->reg;
                    uint8_t mov_instr[] = {0x89, 0xC0}; // MOV reg, EAX template
                    mov_instr[1] = mov_instr[1] + (get_reg_index(target_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, mov_instr, 2);
                    return;
                }
            }
        }
    }

    // Handle REP STOSB - might need to be part of a sequence but output as-is for now
    if (insn->id == X86_INS_STOSB && (insn->detail->x86.prefix[0] == 0xF3 || insn->detail->x86.prefix[1] == 0xF3)) {
        // This instruction is already REP STOSB, just append it
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // For any other case, append original bytes
    buffer_append(b, insn->bytes, insn->size);

    // Verify that no null bytes were introduced by this strategy
    for (size_t i = initial_size; i < b->size; i++) {
        if (b->data[i] == 0x00) {
            fprintf(stderr, "ERROR: Enhanced SALC strategy introduced null at offset %zu (relative offset %zu) in instruction: %s %s\n",
                   i, i - initial_size, insn->mnemonic, insn->op_str);
        }
    }
}

/*
 * Strategy definition
 */
strategy_t salc_conditional_flag_strategy = {
    .name = "Enhanced SALC + Conditional Flag Manipulation + REP STOSB",
    .can_handle = can_handle_salc_conditional_flag,
    .get_size = get_size_salc_conditional_flag,
    .generate = generate_salc_conditional_flag,
    .priority = 93
};