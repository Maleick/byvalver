/*
 * SCASB/CMPSB Conditional Operations Strategy
 *
 * PROBLEM: Conditional operations that check for zero/null bytes can contain nulls
 * in immediate values or comparison operands.
 *
 * SOLUTION: Use SCASB/SCASD/CMPSB/CMPSD string instructions to perform comparisons
 * without using immediate values that contain null bytes.
 *
 * FREQUENCY: Useful in shellcode for string operations and comparisons
 * PRIORITY: 75 (High - efficient string operations without null bytes)
 *
 * Example transformations:
 *   Original: CMP EAX, 0x00000000 (contains nulls in immediate)
 *   Strategy: XOR EBX, EBX; MOV EDI, EAX; SCASD; (ZF set based on comparison)
 */

#include "scasb_cmpsb_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for CMP operations with immediate zero values that contain null bytes
 */
int can_handle_scasb_cmp_zero(cs_insn *insn) {
    if (insn->id != X86_INS_CMP ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be CMP register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate is zero (which would be comparing against zero)
    uint32_t imm = (uint32_t)src_op->imm;
    if (imm != 0) {
        return 0;
    }

    // Check if the original instruction encoding contains nulls
    for (size_t j = 0; j < insn->size; j++) {
        if (insn->bytes[j] == 0x00) {
            return 1;
        }
    }

    return 0;
}

/*
 * Size calculation for SCASB-based zero comparison
 * Uses: PUSH registers + MOV EDI + XOR register + SCAS + POP registers
 */
size_t get_size_scasb_cmp_zero(cs_insn *insn) {
    (void)insn; // Unused parameter
    // PUSH EDI (1) + MOV EDI, reg (2) + XOR EAX,EAX (2) + SCASD (1) + POP EDI (1) = 7 bytes
    return 7;
}

/*
 * Generate SCASB-based zero comparison
 * 
 * For CMP reg, 0:
 *   PUSH EDI           ; Save EDI register
 *   MOV EDI, reg       ; Point EDI to value to compare
 *   XOR EAX, EAX       ; Set EAX to 0 for comparison
 *   SCASD              ; Compare [EDI] with EAX, set flags accordingly
 *   POP EDI            ; Restore EDI
 * 
 * After this, flags are set as if we did CMP reg, 0
 */
void generate_scasb_cmp_zero(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    x86_reg cmp_reg = dst_op->reg;

    // PUSH EDI to save the register we'll use
    uint8_t push_edi[] = {0x57};
    buffer_append(b, push_edi, 1);

    // MOV EDI, cmp_reg - move the value to compare to EDI
    uint8_t mov_edi_reg[] = {0x89, 0xC7}; // MOV EDI, reg (where second 3 bits come from reg index)
    mov_edi_reg[1] = 0xC7 | (get_reg_index(cmp_reg) << 3); // Format: 10001001 11 rrr 111 (MOV EDI, reg)
    buffer_append(b, mov_edi_reg, 2);

    // XOR EAX, EAX - zero out EAX (the value we're comparing against)
    uint8_t xor_eax_eax[] = {0x31, 0xC0};
    buffer_append(b, xor_eax_eax, 2);

    // SCASD - String Compare Doubleword - compares [EDI] with EAX and sets flags
    uint8_t scasd[] = {0xAF};
    buffer_append(b, scasd, 1);

    // POP EDI to restore the register
    uint8_t pop_edi[] = {0x5F};
    buffer_append(b, pop_edi, 1);
}

/*
 * Alternative: SCASB for byte comparisons
 */
int can_handle_scasb_cmp_byte_zero(cs_insn *insn) {
    if (insn->id != X86_INS_CMP ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be CMP register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Check if immediate is zero (which would be comparing against zero)
    uint32_t imm = (uint32_t)src_op->imm;
    if (imm != 0) {
        return 0;
    }

    // Check if the operand is a byte-sized operation
    if (dst_op->size == 1) {
        return 1;
    }

    // Check if the original instruction encoding contains nulls
    for (size_t j = 0; j < insn->size; j++) {
        if (insn->bytes[j] == 0x00) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_scasb_cmp_byte_zero(cs_insn *insn) {
    (void)insn; // Unused parameter
    // PUSH EDI + MOV EDI, reg + XOR EAX, EAX + SCASB + POP EDI
    return 7; // Conservative estimate
}

void generate_scasb_cmp_byte_zero(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    x86_reg cmp_reg = dst_op->reg;

    // PUSH EDI to save the register we'll use
    uint8_t push_edi[] = {0x57};
    buffer_append(b, push_edi, 1);

    // MOV EDI, cmp_reg - move the value to compare to EDI
    uint8_t mov_edi_reg[] = {0x89, 0xC7};
    mov_edi_reg[1] = 0xC7 | (get_reg_index(cmp_reg) << 3);
    buffer_append(b, mov_edi_reg, 2);

    // XOR EAX, EAX - zero out EAX (the value we're comparing against)
    uint8_t xor_eax_eax[] = {0x31, 0xC0};
    buffer_append(b, xor_eax_eax, 2);

    // SCASB - String Compare Byte - compares [EDI] with AL and sets flags
    uint8_t scasb[] = {0xAE};
    buffer_append(b, scasb, 1);

    // POP EDI to restore the register
    uint8_t pop_edi[] = {0x5F};
    buffer_append(b, pop_edi, 1);
}

strategy_t scasb_cmp_zero_strategy = {
    .name = "SCASB-based Zero Comparison",
    .can_handle = can_handle_scasb_cmp_zero,
    .get_size = get_size_scasb_cmp_zero,
    .generate = generate_scasb_cmp_zero,
    .priority = 75  // High priority
};

strategy_t scasb_cmp_byte_zero_strategy = {
    .name = "SCASB-based Byte Zero Comparison",
    .can_handle = can_handle_scasb_cmp_byte_zero,
    .get_size = get_size_scasb_cmp_byte_zero,
    .generate = generate_scasb_cmp_byte_zero,
    .priority = 73  // Slightly lower priority
};

void register_scasb_cmpsb_strategies() {
    register_strategy(&scasb_cmp_zero_strategy);
    register_strategy(&scasb_cmp_byte_zero_strategy);
}