/*
 * SALC-Based Zero Register Strategy
 *
 * PROBLEM: Setting registers to zero using MOV reg, 0x00 contains null bytes:
 * - MOV AL, 0x00 → B0 00 (contains 1 null)
 * - MOV EAX, 0x00000000 → B8 00 00 00 00 (contains 4 nulls)
 * - XOR EAX, EAX → 31 C0 (null-free but limited to register zeroing)
 *
 * SOLUTION: Use SALC (Set AL on Carry) to zero AL register, then move to other registers.
 * SALC sets AL to 0x00 if CF=0, or 0xFF if CF=1. Combined with CLC (Clear Carry) for zero.
 *
 * FREQUENCY: Common in 32-bit shellcode for register initialization
 * PRIORITY: 95 (Very High - more efficient than other methods for zeroing registers)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00000000 (B8 00 00 00 00 - contains 4 nulls)
 *   Strategy: CLC; SALC; MOV AH, AL; MOV EAX, 0 (without null immediate) (F8 D6 88 C4 66 B8 00 00 89 C4 - no nulls)
 */

#include "salc_zero_register_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV reg, 0 instructions that can be replaced with SALC-based zeroing
 */
int can_handle_salc_zero_register(cs_insn *insn) {
    // This should handle MOV register, immediate where immediate is 0
    if (insn->id != X86_INS_MOV ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be MOV register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Must be moving immediate 0
    if (src_op->imm != 0) {
        return 0;
    }

    // Check if the original instruction encoding contains null bytes
    for (size_t i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1; // Contains null bytes
        }
    }

    return 0;
}

/*
 * Size calculation for SALC-based register zeroing
 *
 * Transformation uses:
 * - CLC (1 byte: F8) - Clear carry flag
 * - SALC (1 byte: D6) - Set AL based on carry (AL = 0x00)
 * - MOV operations to propagate zero to target register (2-3 bytes each)
 * Total: ~5-7 bytes depending on target register
 */
size_t get_size_salc_zero_register(cs_insn *insn) {
    (void)insn; // Unused parameter

    // CLC (1) + SALC (1) + MOV operations to propagate to target = ~6-7 bytes
    return 7;
}

/*
 * Generate SALC-based register zeroing
 *
 * For any register = 0x00:
 *   CLC   ; Clear carry flag (F8)
 *   SALC  ; Set AL based on carry (D6) → AL = 0x00
 *   MOV [target_reg_lo], AL  ; Move zero to target register's lower byte
 *   MOV [target_reg], 0 (constructed without nulls)  ; Zero out remaining bytes
 */
void generate_salc_zero_register(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    
    // CLC - Clear carry flag
    buffer_write_byte(b, 0xF8);
    
    // SALC - Set AL based on carry (AL = 0x00 when CF=0)
    buffer_write_byte(b, 0xD6);
    
    // Now AL is zero, move it to the target register based on its size
    x86_reg target_reg = dst_op->reg;
    
    // For 32-bit registers, we need to zero out the full register
    if (target_reg == X86_REG_EAX) {
        // XOR EAX, EAX is more efficient for full EAX zeroing, but let's use our approach
        // MOV AH, AL (move AL to AH, so AH is also 0)
        uint8_t mov_ah_al[] = {0x88, 0xC4};  // MOV AH, AL
        buffer_append(b, mov_ah_al, 2);
        
        // MOV EAX, 0 using null-free construction
        generate_mov_eax_imm(b, 0);  // This will handle 0 efficiently
    } else if (target_reg == X86_REG_EBX) {
        // MOV BL, AL (move AL to BL, so BL is 0)
        uint8_t mov_bl_al[] = {0x88, 0xC3};  // MOV BL, AL
        buffer_append(b, mov_bl_al, 2);
        
        // Clear higher bytes using XOR
        uint8_t xor_ebx_ebx[] = {0x31, 0xDB};  // XOR EBX, EBX
        buffer_append(b, xor_ebx_ebx, 2);
    } else if (target_reg == X86_REG_ECX) {
        // MOV CL, AL
        uint8_t mov_cl_al[] = {0x88, 0xC1};  // MOV CL, AL
        buffer_append(b, mov_cl_al, 2);
        
        // Clear with XOR
        uint8_t xor_ecx_ecx[] = {0x31, 0xC9};  // XOR ECX, ECX
        buffer_append(b, xor_ecx_ecx, 2);
    } else if (target_reg == X86_REG_EDX) {
        // MOV DL, AL
        uint8_t mov_dl_al[] = {0x88, 0xC2};  // MOV DL, AL
        buffer_append(b, mov_dl_al, 2);
        
        // Clear with XOR
        uint8_t xor_edx_edx[] = {0x31, 0xD2};  // XOR EDX, EDX
        buffer_append(b, xor_edx_edx, 2);
    } else {
        // For other registers, use a more generic approach
        // MOV [low_byte_of_target], AL
        uint8_t reg_idx = get_reg_index(target_reg);
        if (reg_idx <= 3) {  // Only for registers where low-byte access is valid (AL, CL, DL, BL)
            if (reg_idx < 4) {
                uint8_t mov_cmd[] = {0x88, 0xC0 | reg_idx};  // MOV reg_low_byte, AL
                buffer_append(b, mov_cmd, 2);
            }
        }
        
        // Use generic zeroing approach: XOR reg, reg
        // Determine the appropriate XOR instruction based on the register
        if (reg_idx < 8) {
            uint8_t xor_cmd[] = {0x31, 0xC0 | (reg_idx << 3) | reg_idx}; // XOR reg, reg
            buffer_append(b, xor_cmd, 2);
        }
    }
}

/*
 * Alternative strategy: Use SALC for AL zeroing specifically, with high priority
 */
int can_handle_salc_zero_al_direct(cs_insn *insn) {
    if (insn->id != X86_INS_MOV ||
        insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be MOV register, immediate
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Must be moving to AL register specifically
    if (dst_op->reg != X86_REG_AL) {
        return 0;
    }

    // Must be moving immediate 0
    if (src_op->imm != 0) {
        return 0;
    }

    // Check if the original instruction encoding contains null bytes
    for (size_t i = 0; i < insn->size; i++) {
        if (insn->bytes[i] == 0x00) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_salc_zero_al_direct(cs_insn *insn) {
    (void)insn;
    // CLC (1) + SALC (1) = 2 bytes
    return 2;
}

void generate_salc_zero_al_direct(struct buffer *b, cs_insn *insn) {
    (void)insn;
    // CLC - Clear carry flag
    buffer_write_byte(b, 0xF8);
    // SALC - Set AL based on carry (AL = 0x00 when CF=0)
    buffer_write_byte(b, 0xD6);
}

strategy_t salc_zero_register_strategy = {
    .name = "SALC Register Zeroing",
    .can_handle = can_handle_salc_zero_register,
    .get_size = get_size_salc_zero_register,
    .generate = generate_salc_zero_register,
    .priority = 95  // Very high priority
};

strategy_t salc_zero_al_direct_strategy = {
    .name = "SALC AL Direct Zeroing",
    .can_handle = can_handle_salc_zero_al_direct,
    .get_size = get_size_salc_zero_al_direct,
    .generate = generate_salc_zero_al_direct,
    .priority = 98  // Highest priority for AL zeroing
};

void register_salc_zero_register_strategies() {
    register_strategy(&salc_zero_register_strategy);
    register_strategy(&salc_zero_al_direct_strategy);
}