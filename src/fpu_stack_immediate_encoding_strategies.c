/*
 * FPU Stack-Based Immediate Encoding Strategies
 *
 * PROBLEM: The x87 Floating-Point Unit (FPU) stack provides an alternative data 
 * storage mechanism that can be exploited for encoding integer values and avoiding 
 * bad characters in GPR operations.
 *
 * SOLUTION: Use FPU operations like FILD (Float Integer Load) from memory and 
 * FISTP (Store Integer and Pop) to transfer values between integer and FPU domains.
 */

#include "fpu_stack_immediate_encoding_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Strategy registry entry
strategy_t fpu_stack_immediate_encoding_strategy = {
    .name = "FPU Stack-Based Immediate Encoding",
    .can_handle = can_handle_fpu_stack_immediate_encoding,
    .get_size = get_size_fpu_stack_immediate_encoding,
    .generate = generate_fpu_stack_immediate_encoding,
    .priority = 76
};

// Helper function to check if an instruction has bad characters in its encoding
static int instruction_has_bad_chars(cs_insn *insn) {
    if (!insn || !insn->bytes) {
        return 0;
    }
    
    for (int i = 0; i < insn->size; i++) {
        if (!is_bad_char_free_byte(insn->bytes[i])) {
            return 1;
        }
    }
    return 0;
}

// Check if this is an instruction that can benefit from FPU encoding
int can_handle_fpu_stack_immediate_encoding(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if this is a MOV immediate instruction that has bad characters
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // Must be: MOV reg32, imm32
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // Check if destination is a 32-bit register
            if (dst_op->size == 4) {
                // Check if the instruction has bad characters
                if (instruction_has_bad_chars(insn)) {
                    return 1;
                }
                
                // Also check if the immediate value itself contains bad characters
                uint32_t imm = (uint32_t)src_op->imm;
                uint8_t *imm_bytes = (uint8_t*)&imm;
                for (int i = 0; i < 4; i++) {
                    if (!is_bad_char_free_byte(imm_bytes[i])) {
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

// Estimate the size of the transformed instruction
size_t get_size_fpu_stack_immediate_encoding(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Conservative estimate: FPU encoding typically requires 15-25 bytes
    // push imm32 (5 bytes) + fild (2-3 bytes) + fistp (2-3 bytes) + pop reg (1-2 bytes)
    return 20;
}

// Generate the transformed instruction using FPU stack
void generate_fpu_stack_immediate_encoding(struct buffer *b, cs_insn *insn) {
    if (!b || !insn) {
        return;
    }

    // Check if this is a MOV reg32, imm32 instruction
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM && dst_op->size == 4) {
            uint32_t imm = (uint32_t)src_op->imm;
            
            // Transform: MOV reg, immediate
            // To: push immediate; fild dword [esp]; fistp dword [esp]; mov reg, [esp]; add esp, 4
            // Or more directly: push immediate; fild dword [esp]; fistp dword [esp]; pop reg
            
            // push immediate value onto stack
            buffer_append(b, (uint8_t[]){0x68}, 1);  // PUSH imm32
            buffer_append(b, (uint8_t*)&imm, 4);     // Immediate value
            
            // fild dword ptr [esp] - load integer from stack to FPU ST(0)
            buffer_append(b, (uint8_t[]){0xDB, 0x04, 0x24}, 3);  // FILD dword ptr [ESP]
            
            // fistp dword ptr [esp] - store integer from FPU ST(0) to stack and pop
            buffer_append(b, (uint8_t[]){0xDB, 0x1C, 0x24}, 3);  // FISTP dword ptr [ESP]
            
            // pop into destination register
            uint8_t reg_idx = get_reg_index(dst_op->reg);
            if (reg_idx <= 7) {  // Ensure it's a valid register index
                uint8_t pop_reg_insn[] = {0x58 + reg_idx};  // POP EAX/ECX/EDX/EBX/ESP/EBP/ESI/EDI
                buffer_append(b, pop_reg_insn, 1);
            } else {
                // Fallback if register index is invalid
                // Use a temporary register (EAX) and then move
                buffer_append(b, (uint8_t[]){0x58}, 1);  // POP EAX
                // MOV dst_reg, EAX
                uint8_t modrm = (3 << 6) | (get_reg_index(dst_op->reg) << 3) | 0;  // reg=dst, r/m=EAX
                uint8_t mov_insn[] = {0x89, modrm};
                buffer_append(b, mov_insn, 2);
            }
        }
    }
}

// Registration function
void register_fpu_stack_immediate_encoding_strategies(void) {
    extern strategy_t fpu_stack_immediate_encoding_strategy;
    register_strategy(&fpu_stack_immediate_encoding_strategy);
}