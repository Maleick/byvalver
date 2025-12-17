/*
 * Stack-Based Structure Construction Strategy
 *
 * PROBLEM: Advanced shellcode constructs complex Windows structures (like STARTUPINFO,
 * PROCESS_INFORMATION, sockaddr_in, etc.) directly on the stack during runtime.
 * This technique avoids hardcoded structures that might contain null bytes or be 
 * easily detectable by static analysis. The construction process may involve multiple
 * PUSH operations or MOV operations with immediate values that contain nulls.
 *
 * SOLUTION: Implement a strategy to handle the runtime construction of complex structures
 * on the stack while eliminating any null bytes that may appear in immediate values
 * or displacement fields during the construction process.
 *
 * FREQUENCY: Common in shellcode that creates processes or network connections
 * PRIORITY: 94 (High - important for process/network shellcode)
 *
 * Example transformations:
 *   Structure construction: Multiple PUSH operations building a structure on stack
 *   Strategy: Maintain structure construction pattern while eliminating null bytes
 */

#include "stack_based_structure_construction.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Handle structure construction patterns that may contain nulls
 */
int can_handle_stack_structure_construction(cs_insn *insn) {
    if (!insn || !insn->detail) return 0;

    // Look for PUSH operations that might be part of structure construction
    if (insn->id == X86_INS_PUSH) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            // Check if immediate value contains nulls
            if (!is_bad_char_free((uint32_t)op->imm)) {
                return 1;
            }
        }
    }

    // Look for MOV operations to stack addresses that might be part of structure building
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // Check for MOV [ESP + offset], immediate - common in structure construction
        if (dst_op->type == X86_OP_MEM && src_op->type == X86_OP_IMM) {
            if (dst_op->mem.base == X86_REG_ESP || dst_op->mem.base == X86_REG_RSP) {
                // This is a stack-based memory write, likely part of structure construction
                if (!is_bad_char_free((uint32_t)src_op->imm)) {
                    return 1;
                }
            }
        }

        // Also check for MOV reg, immediate where immediate contains nulls
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                // This might be part of a sequence to build structure data in registers
                // before moving to stack
                return 1;
            }
        }
    }

    // Look for SUB/ADD operations on ESP/RSP that adjust stack for structure allocation
    if ((insn->id == X86_INS_SUB || insn->id == X86_INS_ADD) && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if ((dst_op->reg == X86_REG_ESP || dst_op->reg == X86_REG_RSP) && src_op->type == X86_OP_IMM) {
            // Stack pointer adjustment, often for structure allocation
            // Check if immediate contains nulls
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Size calculation function for stack-based structure construction
 */
size_t get_size_stack_structure_construction(cs_insn *insn) {
    (void)insn; // Unused parameter
    // Structure construction varies, but typically requires more space for null-safe ops
    return 8; // Estimate based on potential register save/restore operations
}

/*
 * Generation function for null-free stack-based structure construction
 */
void generate_stack_structure_construction_null_free(struct buffer *b, cs_insn *insn) {
    // Store the initial size to verify no nulls are introduced
    size_t initial_size = b->size;

    if (insn->id == X86_INS_PUSH) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)op->imm;
            
            if (!is_bad_char_free(imm)) {
                // Use null-safe approach: MOV EAX, imm; PUSH EAX
                generate_mov_eax_imm(b, imm);
                uint8_t push_eax[] = {0x50 + get_reg_index(X86_REG_EAX)}; // PUSH EAX
                buffer_append(b, push_eax, 1);
            } else {
                // Normal PUSH immediate
                generate_push_imm(b, imm);
            }
            return;
        }
    } else if (insn->id == X86_INS_MOV) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_MEM && src_op->type == X86_OP_IMM) {
            // MOV [ESP + offset], immediate - common in structure construction
            if (dst_op->mem.base == X86_REG_ESP || dst_op->mem.base == X86_REG_RSP) {
                uint32_t imm = (uint32_t)src_op->imm;
                
                if (!is_bad_char_free(imm)) {
                    // Use null-safe approach: MOV EAX, imm; MOV [ESP+offset], EAX
                    generate_mov_eax_imm(b, imm);
                    
                    // Calculate MOD R/M byte for [ESP + disp8/32]
                    uint8_t mod_rm = 0x44; // MOD=01, reg=EAX, R/M=SIB
                    uint8_t sib = 0x24;    // Base=ESP, Index=none, Scale=1
                    buffer_append(b, &mod_rm, 1);
                    buffer_append(b, &sib, 1);
                    buffer_write_dword(b, dst_op->mem.disp);
                } else {
                    // Generate normal MOV with displacement, but check if displacement itself has nulls
                    if (!is_bad_char_free(dst_op->mem.disp)) {
                        // If displacement has nulls, use register-based approach
                        // MOV ECX, imm (source value)
                        generate_mov_eax_imm(b, imm); // MOV EAX, imm (value to store)

                        // Now we need to store to [ESP + disp] where disp might have nulls
                        // We'll use LEA to calculate the target address in a register
                        // LEA EDI, [ESP + disp] - but if disp has nulls, this won't work
                        // So we need to do: MOV EDI, ESP; ADD EDI, disp
                        uint8_t mov_edi_esp[] = {0x89, 0xE7}; // MOV EDI, ESP
                        buffer_append(b, mov_edi_esp, 2);

                        // ADD EDI, disp - but disp might have nulls, so we'll use MOV + ADD
                        // Use ESI as temporary register to hold the displacement
                        generate_mov_eax_imm(b, dst_op->mem.disp); // MOV EAX, disp (if it has nulls)
                        uint8_t xchg_edi_eax[] = {0x97}; // XCHG EDI, EAX (swap EDI and EAX)
                        buffer_append(b, xchg_edi_eax, 1);
                        uint8_t add_edi_eax[] = {0x01, 0xC7}; // ADD EDI, EAX
                        buffer_append(b, add_edi_eax, 2);

                        // Now do MOV [EDI], EAX to store the value
                        uint8_t mov_indirect[] = {0x89, 0x07}; // MOV [EDI], EAX
                        buffer_append(b, mov_indirect, 2);
                    } else {
                        // Normal MOV [ESP + disp], imm32
                        // Format: MOV imm32, [base + disp8/32]
                        uint8_t mov_instr[11] = {0xC7, 0x84, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // MOV [ESP + disp32], imm32
                        mov_instr[3] = dst_op->mem.disp & 0xFF;
                        mov_instr[4] = (dst_op->mem.disp >> 8) & 0xFF;
                        mov_instr[5] = (dst_op->mem.disp >> 16) & 0xFF;
                        mov_instr[6] = (dst_op->mem.disp >> 24) & 0xFF;
                        mov_instr[7] = imm & 0xFF;
                        mov_instr[8] = (imm >> 8) & 0xFF;
                        mov_instr[9] = (imm >> 16) & 0xFF;
                        mov_instr[10] = (imm >> 24) & 0xFF;

                        buffer_append(b, mov_instr, 11);
                    }
                }
                return;
            }
        } else if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // MOV reg, immediate for structure construction in registers
            uint32_t imm = (uint32_t)src_op->imm;

            if (!is_bad_char_free(imm)) {
                // Use null-safe MOV generation
                uint8_t dst_reg = dst_op->reg;

                if (dst_reg == X86_REG_EAX) {
                    generate_mov_eax_imm(b, imm);
                } else {
                    // Save original register, load via EAX, then move
                    uint8_t push_code[] = {0x50 + get_reg_index(dst_reg)}; // PUSH reg
                    buffer_append(b, push_code, 1);

                    generate_mov_eax_imm(b, imm);  // Load value to EAX (null-safe)

                    uint8_t mov_reg_eax[] = {0x89, 0xC0}; // MOV reg, EAX
                    mov_reg_eax[1] = mov_reg_eax[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, mov_reg_eax, 2);

                    uint8_t pop_code[] = {0x58 + get_reg_index(dst_reg)}; // POP reg
                    buffer_append(b, pop_code, 1);
                }
            } else {
                // No nulls, use normal MOV
                generate_mov_reg_imm(b, insn);
            }
            return;
        }
    } else if (insn->id == X86_INS_SUB || insn->id == X86_INS_ADD) {
        // Handle ESP/RSP adjustments for structure allocation
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if ((dst_op->reg == X86_REG_ESP || dst_op->reg == X86_REG_RSP) && src_op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)src_op->imm;
            
            if (!is_bad_char_free(imm)) {
                // Use null-safe approach: MOV EAX, imm; ADD ESP, EAX (or SUB ESP, EAX)
                generate_mov_eax_imm(b, imm);
                
                if (insn->id == X86_INS_ADD) {
                    uint8_t add_esp_eax[] = {0x01, 0xC4}; // ADD ESP, EAX
                    buffer_append(b, add_esp_eax, 2);
                } else { // SUB
                    uint8_t sub_esp_eax[] = {0x29, 0xC4}; // SUB ESP, EAX
                    buffer_append(b, sub_esp_eax, 2);
                }
            } else {
                // Normal operation - but we don't have direct functions for ADD/SUB reg, imm
                // So we'll use the same approach: MOV EAX, imm; ADD/SUB ESP, EAX
                generate_mov_eax_imm(b, imm);
                if (insn->id == X86_INS_SUB) {
                    uint8_t sub_esp_eax[] = {0x29, 0xC4}; // SUB ESP, EAX
                    buffer_append(b, sub_esp_eax, 2);
                } else { // ADD
                    uint8_t add_esp_eax[] = {0x01, 0xC4}; // ADD ESP, EAX
                    buffer_append(b, add_esp_eax, 2);
                }
            }
            return;
        }
    }

    // For remaining cases, use standard utilities with null-safety
    switch(insn->id) {
        case X86_INS_MOV:
            generate_mov_reg_imm(b, insn);
            break;
        case X86_INS_PUSH:
            // For PUSH that wasn't handled above
            {
                cs_x86_op *op = &insn->detail->x86.operands[0];
                if (op->type == X86_OP_IMM && !is_bad_char_free((uint32_t)op->imm)) {
                    generate_mov_eax_imm(b, (uint32_t)op->imm);
                    uint8_t push_eax[] = {0x50 + get_reg_index(X86_REG_EAX)}; // PUSH EAX
                    buffer_append(b, push_eax, 1);
                } else {
                    buffer_append(b, insn->bytes, insn->size);
                }
            }
            break;
        case X86_INS_ADD:
        case X86_INS_SUB:
            // Handle arithmetic with possible null bytes in immediates
            if (has_null_bytes(insn)) {
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                cs_x86_op *src_op = &insn->detail->x86.operands[1];
                
                if (src_op->type == X86_OP_IMM) {
                    uint32_t imm = (uint32_t)src_op->imm;
                    uint8_t dst_reg = dst_op->reg;
                    
                    // Save the destination register if needed
                    if (dst_reg != X86_REG_EAX) {
                        uint8_t push_code[] = {0x50 + get_reg_index(dst_reg)}; // PUSH dst_reg
                        buffer_append(b, push_code, 1);
                    }

                    // Load the immediate value to EAX using null-safe method
                    generate_mov_eax_imm(b, imm);

                    // Perform the operation: op dst_reg, EAX
                    uint8_t op_code = 0x01; // Default to ADD
                    switch(insn->id) {
                        case X86_INS_ADD: op_code = 0x01; break;  // ADD
                        case X86_INS_SUB: op_code = 0x29; break;  // SUB
                    }

                    uint8_t op_instr[] = {op_code, 0xC0}; // op reg, EAX
                    op_instr[1] = op_instr[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, op_instr, 2);

                    // Restore the destination register if it was saved
                    if (dst_reg != X86_REG_EAX) {
                        uint8_t pop_code[] = {0x58 + get_reg_index(dst_reg)}; // POP dst_reg
                        buffer_append(b, pop_code, 1);
                    }
                } else {
                    // Non-immediate operand, use standard generation
                    buffer_append(b, insn->bytes, insn->size);
                }
            } else {
                buffer_append(b, insn->bytes, insn->size);
            }
            break;
        default:
            // For other instruction types, append original bytes
            buffer_append(b, insn->bytes, insn->size);
            break;
    }

    // Verify that no null bytes were introduced by this strategy
    for (size_t i = initial_size; i < b->size; i++) {
        if (b->data[i] == 0x00) {
            fprintf(stderr, "ERROR: Stack structure strategy introduced null at offset %zu (relative offset %zu) in instruction: %s %s\n",
                   i, i - initial_size, insn->mnemonic, insn->op_str);
        }
    }
}

// Define the strategy structure
strategy_t stack_structure_construction_strategy = {
    .name = "Stack-Based Structure Construction Strategy",
    .can_handle = can_handle_stack_structure_construction,
    .get_size = get_size_stack_structure_construction,
    .generate = generate_stack_structure_construction_null_free,
    .priority = 94  // High priority for structure construction strategy
};

// Function to register stack-based structure construction strategies
void register_stack_based_structure_construction_strategies() {
    register_strategy(&stack_structure_construction_strategy);
}