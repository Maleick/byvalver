/*
 * Stack-Based String Construction Strategy
 *
 * PROBLEM: Shellcode often needs to construct strings (like DLL names, API names,
 * file paths, etc.) directly on the stack during runtime. This is done to avoid
 * hardcoded strings that might contain null bytes or be easily detectable. 
 * The construction typically involves multiple PUSH operations with immediate
 * values that represent string chunks, MOV operations to set up string contents,
 * or SUB operations to allocate stack space, then adjusting the stack pointer 
 * to point to the beginning of the constructed string.
 *
 * SOLUTION: Implement a strategy to handle the runtime construction of strings
 * on the stack while eliminating any null bytes that may appear in the string
 * chunks or other immediate values during the construction process.
 *
 * FREQUENCY: Common in shellcode that loads DLLs or calls APIs with string parameters
 * PRIORITY: 95 (Very High - common evasion technique)
 *
 * Example transformations:
 *   String construction: Multiple PUSH operations building "ws2_32" on stack
 *   Strategy: Maintain string construction pattern while eliminating null bytes
 */

#include "stack_string_construction_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for stack-based string construction patterns that may contain nulls
 */
int can_handle_stack_string_construction(cs_insn *insn) {
    if (!insn || !insn->detail) return 0;

    // Look for PUSH operations that might be part of string construction
    if (insn->id == X86_INS_PUSH) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)op->imm;
            
            // Check if the immediate contains null bytes
            if (!is_bad_char_free(imm)) {
                return 1;
            }

            // Check if this looks like it might be a string (ASCII printable range)
            uint8_t bytes[4];
            memcpy(bytes, &imm, 4);

            int likely_string = 0;
            for (int i = 0; i < 4; i++) {
                // Count printable ASCII characters (excluding nulls)
                if ((bytes[i] >= 0x20 && bytes[i] <= 0x7E) || bytes[i] == 0) {
                    likely_string++;
                }
            }

            // If at least 2 out of 4 bytes are printable, consider it a string
            return (likely_string >= 2) ? 1 : 0;
        }
    }

    // Look for MOV operations to stack addresses that might be part of string building
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // Check for MOV [ESP + offset], immediate - common in string construction
        if (dst_op->type == X86_OP_MEM && src_op->type == X86_OP_IMM) {
            if (dst_op->mem.base == X86_REG_ESP || dst_op->mem.base == X86_REG_RSP) {
                // This is a stack-based memory write, might be part of string construction
                if (!is_bad_char_free((uint32_t)src_op->imm)) {
                    return 1;
                }
            }
        }

        // Also check for MOV reg, immediate where immediate contains nulls
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    // Look for SUB operations on ESP/RSP that might be for string allocation
    if (insn->id == X86_INS_SUB && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if ((dst_op->reg == X86_REG_ESP || dst_op->reg == X86_REG_RSP) && src_op->type == X86_OP_IMM) {
            // Stack pointer subtraction, often for string buffer allocation
            if (!is_bad_char_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    // Look for CALL operations that might be using strings constructed on stack
    if (insn->id == X86_INS_CALL && insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t target = (uint32_t)op->imm;
            if (!is_bad_char_free(target)) {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Size calculation function for stack-based string construction
 */
size_t get_size_stack_string_construction(cs_insn *insn) {
    (void)insn; // Unused parameter
    // String construction varies but typically requires more space for null-safe operations
    return 6; // Estimate based on potential register save/restore operations
}

/*
 * Generation function for null-free stack-based string construction
 */
void generate_stack_string_construction(struct buffer *b, cs_insn *insn) {
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
                generate_push_imm32(b, imm);
            }
            return;
        }
    } else if (insn->id == X86_INS_MOV) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (dst_op->type == X86_OP_MEM && src_op->type == X86_OP_IMM) {
            // MOV [ESP + offset], immediate - potentially part of string construction
            if (dst_op->mem.base == X86_REG_ESP || dst_op->mem.base == X86_REG_RSP) {
                uint32_t imm = (uint32_t)src_op->imm;
                
                if (!is_bad_char_free(imm)) {
                    // Use null-safe approach: MOV EAX, imm; MOV [ESP+offset], EAX
                    generate_mov_eax_imm(b, imm);
                    
                    // Create MOV [ESP + disp32], EAX instruction
                    // Format: 89 /r where /r specifies the mod/rm byte
                    uint8_t mod_rm = 0x84; // MOD=10 (disp32), reg=EAX in r part
                    uint8_t sib = 0x24;    // SIB: no index, ESP base
                    buffer_append(b, &mod_rm, 1); // MOD R/M byte
                    buffer_append(b, &sib, 1);    // SIB byte
                    buffer_write_dword(b, dst_op->mem.disp); // 32-bit displacement
                } else {
                    // Check if displacement has nulls
                    if (!is_bad_char_free(dst_op->mem.disp)) {
                        // If displacement has nulls, use register-based approach
                        // MOV EDI, ESP; ADD EDI, disp; MOV [EDI], imm
                        uint8_t mov_edi_esp[] = {0x89, 0xE7}; // MOV EDI, ESP
                        buffer_append(b, mov_edi_esp, 2);
                        
                        // MOV EAX, disp (for displacement)
                        generate_mov_eax_imm(b, dst_op->mem.disp);
                        // ADD EDI, EAX (add displacement to ESP)
                        uint8_t add_edi_eax[] = {0x01, 0xC7}; // ADD EDI, EAX
                        buffer_append(b, add_edi_eax, 2);
                        
                        // MOV EAX, imm (the value to store)
                        generate_mov_eax_imm(b, imm);
                        // MOV [EDI], EAX (store value at calculated address)
                        uint8_t mov_to_target[] = {0x89, 0x07}; // MOV [EDI], EAX
                        buffer_append(b, mov_to_target, 2);
                    } else {
                        // Check if any of the displacement bytes have nulls
                        if (!is_bad_char_free(dst_op->mem.disp)) {
                            // If disp32 has nulls, use the complex approach 
                            // MOV EDI, ESP; MOV EAX, disp32; ADD EDI, EAX; MOV [EDI], original_imm
                            uint8_t mov_edi_esp[] = {0x89, 0xE7}; // MOV EDI, ESP  
                            buffer_append(b, mov_edi_esp, 2);
                            
                            // MOV EAX, disp for displacement
                            generate_mov_eax_imm(b, dst_op->mem.disp);
                            uint8_t add_edi_eax[] = {0x01, 0xC7}; // ADD EDI, EAX  
                            buffer_append(b, add_edi_eax, 2);
                            
                            // MOV EAX, original immediate value
                            generate_mov_eax_imm(b, imm);
                            uint8_t mov_to_target[] = {0x89, 0x07}; // MOV [EDI], EAX
                            buffer_append(b, mov_to_target, 2);
                        } else {
                            // Normal MOV [ESP + disp], imm32
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
                }
                return;
            }
        } else if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_IMM) {
            // MOV reg, immediate for string construction in registers
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
    } else if (insn->id == X86_INS_SUB) {
        // Handle ESP/RSP subtractions for string buffer allocation
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if ((dst_op->reg == X86_REG_ESP || dst_op->reg == X86_REG_RSP) && src_op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)src_op->imm;
            
            if (!is_bad_char_free(imm)) {
                // Use null-safe approach: MOV EAX, imm; SUB ESP, EAX
                generate_mov_eax_imm(b, imm);
                uint8_t sub_esp_eax[] = {0x29, 0xC4}; // SUB ESP, EAX
                buffer_append(b, sub_esp_eax, 2);
            } else {
                // Use null-safe approach anyway: MOV EAX, imm; SUB ESP, EAX (to maintain consistency)
                generate_mov_eax_imm(b, imm);
                uint8_t sub_esp_eax[] = {0x29, 0xC4}; // SUB ESP, EAX
                buffer_append(b, sub_esp_eax, 2);
            }
            return;
        }
    } else if (insn->id == X86_INS_CALL) {
        // Handle CALL with immediate that might contain nulls
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t target = (uint32_t)op->imm;
            if (!is_bad_char_free(target)) {
                // Transform CALL immediate to CALL register to avoid nulls in immediate
                // MOV EAX, target_address (null-free)
                generate_mov_eax_imm(b, target);
                // CALL EAX
                uint8_t call_eax[] = {0xFF, 0xD0};
                buffer_append(b, call_eax, 2);
                return;
            }
        }
    }

    // For remaining cases, use standard utilities with null-safety
    switch(insn->id) {
        case X86_INS_MOV:
            // For MOV not handled above
            if (has_null_bytes(insn)) {
                // Apply null-safe MOV logic
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                cs_x86_op *src_op = &insn->detail->x86.operands[1];
                
                if (src_op->type == X86_OP_IMM && dst_op->type == X86_OP_REG) {
                    uint32_t imm = (uint32_t)src_op->imm;
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
                    buffer_append(b, insn->bytes, insn->size);
                }
            } else {
                buffer_append(b, insn->bytes, insn->size);
            }
            break;
        case X86_INS_PUSH:
            // For PUSH not handled above
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
        case X86_INS_SUB:
            // For SUB not handled above
            {
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                cs_x86_op *src_op = &insn->detail->x86.operands[1];
                
                if (dst_op->type == X86_OP_REG && 
                    (dst_op->reg == X86_REG_ESP || dst_op->reg == X86_REG_RSP) && 
                    src_op->type == X86_OP_IMM) {
                    // Use null-safe approach for stack subtractions
                    generate_mov_eax_imm(b, (uint32_t)src_op->imm);
                    uint8_t sub_instr[] = {0x29, 0xC4}; // SUB ESP, EAX
                    buffer_append(b, sub_instr, 2);
                } else {
                    buffer_append(b, insn->bytes, insn->size);
                }
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
            fprintf(stderr, "ERROR: Stack string strategy introduced null at offset %zu (relative offset %zu) in instruction: %s %s\n",
                   i, i - initial_size, insn->mnemonic, insn->op_str);
        }
    }
}

/*
 * Strategy definition
 */
strategy_t stack_string_construction_strategy = {
    .name = "Advanced Stack-Based String Construction Strategy",
    .can_handle = can_handle_stack_string_construction,
    .get_size = get_size_stack_string_construction,
    .generate = generate_stack_string_construction,
    .priority = 95  // Very high priority for string construction strategy
};