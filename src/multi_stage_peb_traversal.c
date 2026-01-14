/*
 * Multi-Stage PEB Traversal Strategy
 *
 * PROBLEM: Advanced shellcode often uses multi-stage PEB (Process Environment Block) 
 * traversal to load multiple DLLs sequentially (e.g., kernel32, user32, ws2_32) before
 * resolving APIs. This technique is more sophisticated than single-DLL resolution
 * and requires special handling to maintain null-byte elimination.
 *
 * SOLUTION: Implement a strategy to handle complex PEB traversal sequences that load
 * multiple DLLs and resolve APIs from different modules in a single shellcode.
 *
 * FREQUENCY: Common in complex shellcode payloads
 * PRIORITY: 97 (Critical - advanced evasion technique)
 *
 * Example transformations:
 *   Multi-stage PEB: Load kernel32 -> resolve LoadLibraryA -> load user32 -> resolve APIs
 *   Strategy: Preserve the multi-stage loading pattern while eliminating null bytes
 */

#include "multi_stage_peb_traversal.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for multi-stage PEB traversal patterns
 */
int can_handle_multi_stage_peb_traversal(cs_insn *insn) {
    if (!insn || !insn->detail) return 0;

    // Look for CALL instructions that might be LoadLibraryA calls in PEB sequences
    if (insn->id == X86_INS_CALL && insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        // Check for CALL register (common in PEB resolution)
        if (op->type == X86_OP_REG) {
            // This could be part of a PEB traversal sequence
            return 1;
        }
        
        // Check for CALL immediate that may have nulls (resolved PEB addresses)
        if (op->type == X86_OP_IMM) {
            uint32_t target = (uint32_t)op->imm;
            if (!is_bad_byte_free(target)) {
                return 1;
            }
        }
    }

    // Look for MOV operations that might be part of PEB traversal
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // MOV reg, [reg + offset] - common in PEB traversal (like fs:[0x30])
        if (dst_op->type == X86_OP_REG && src_op->type == X86_OP_MEM) {
            if (src_op->mem.segment == X86_REG_FS && src_op->mem.disp == 0x30) {
                // This is accessing PEB (fs:[0x30])
                return 1;
            }
            // Additional PEB-related offsets: 0x0c (PEB_LDR_DATA), 0x08 (ImageBaseAddress)
            if (src_op->mem.segment == X86_REG_FS && (src_op->mem.disp == 0x0c || src_op->mem.disp == 0x08)) {
                return 1;
            }
        }

        // Check for MOV with immediate that may contain nulls (part of API resolution)
        if (src_op->type == X86_OP_IMM) {
            if (!is_bad_byte_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    // Look for string operations that might be DLL/API names in PEB sequences
    if ((insn->id == X86_INS_PUSH || insn->id == X86_INS_MOV) && 
        insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            // This could be pushing a DLL name or API hash
            if (!is_bad_byte_free((uint32_t)op->imm)) {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Size calculation function for multi-stage PEB traversal
 */
size_t get_size_multi_stage_peb_traversal(cs_insn *insn) {
    (void)insn; // Unused parameter
    // PEB traversal sequences are typically complex and vary in size
    return 20; // Conservative estimate for multi-stage operations
}

/*
 * Generation function for multi-stage PEB traversal with null-byte elimination
 */
void generate_multi_stage_peb_traversal_null_free(struct buffer *b, cs_insn *insn) {
    // Store the initial size to verify no nulls are introduced
    size_t initial_size = b->size;

    if (insn->id == X86_INS_MOV) {
        // Handle MOV instruction that may contain nulls
        cs_x86_op *dst_op = &insn->detail->x86.operands[0];
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        if (src_op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)src_op->imm;

            if (!is_bad_byte_free(imm)) {
                // Use null-safe MOV generation for immediate values containing nulls
                uint8_t dst_reg = dst_op->reg;

                if (dst_reg == X86_REG_EAX) {
                    // Directly use generate_mov_eax_imm for EAX
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
                // No nulls in immediate, use normal MOV
                generate_mov_reg_imm(b, insn);
            }
            return;
        } else if (src_op->type == X86_OP_MEM) {
            // Handle memory operations in PEB traversal (e.g., MOV EAX, [FS:0x30])
            // These operations are typically null-safe, so generate normally
            uint8_t opcodes[8];
            size_t size = 0;
            
            // Handle FS segment override if present
            if (src_op->mem.segment == X86_REG_FS) {
                opcodes[size++] = 0x64; // FS segment override
            }
            
            // Add the MOV instruction
            uint8_t dst_reg = dst_op->reg;
            uint8_t mod_rm = 0x00; // [base + disp32]
            
            // Calculate the MOD R/M and SIB bytes
            mod_rm = (dst_reg << 3) | 0x05; // [disp32] addressing
            
            opcodes[size++] = 0x8B; // MOV reg, [r/m]
            opcodes[size++] = mod_rm;
            
            // Add the displacement (4 bytes for disp32)
            uint32_t disp = src_op->mem.disp;
            
            // Check if the displacement contains nulls
            if (!is_bad_byte_free(disp)) {
                // If the displacement has nulls, we need to load it differently
                // Approach: MOV EAX, disp32; MOV reg, [EAX]
                generate_mov_eax_imm(b, disp);  // Load displacement to EAX
                
                // Now do MOV dst_reg, [EAX]
                uint8_t indirect_mov[] = {0x8B, 0x00}; // MOV reg, [EAX]
                indirect_mov[1] = indirect_mov[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                buffer_append(b, indirect_mov, 2);
            } else {
                // Direct approach: MOV reg, [disp32]
                buffer_append(b, opcodes, size);
                
                // Add the displacement bytes directly
                buffer_write_dword(b, disp);
            }
            return;
        }
    } else if (insn->id == X86_INS_CALL) {
        // Handle CALL with register or immediate that might have nulls
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t target = (uint32_t)op->imm;
            if (!is_bad_byte_free(target)) {
                // Transform CALL immediate to CALL register to avoid nulls in immediate
                // MOV EAX, target_address (null-free)
                generate_mov_eax_imm(b, target);
                // CALL EAX
                uint8_t call_eax[] = {0xFF, 0xD0};
                buffer_append(b, call_eax, 2);
                return;
            }
        } else if (op->type == X86_OP_REG) {
            // CALL register - this is normal for resolved PEB API calls
            // Just output the call instruction safely
            uint8_t call_reg[] = {0xFF, 0xD0}; // CALL EAX as template
            call_reg[1] = call_reg[1] + get_reg_index(op->reg);
            buffer_append(b, call_reg, 2);
            return;
        }
    } else if (insn->id == X86_INS_PUSH) {
        // Handle PUSH operations that might be part of PEB sequences
        cs_x86_op *op = &insn->detail->x86.operands[0];
        
        if (op->type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)op->imm;
            
            if (!is_bad_byte_free(imm)) {
                // Use null-safe approach: MOV EAX, imm; PUSH EAX
                generate_mov_eax_imm(b, imm);
                uint8_t push_eax[] = {0x50}; // PUSH EAX
                push_eax[0] = push_eax[0] + get_reg_index(X86_REG_EAX);
                buffer_append(b, push_eax, 1);
                return;
            } else {
                // Normal PUSH immediate
                generate_push_imm(b, imm);
                return;
            }
        }
    }

    // For remaining cases, use standard utilities with null-safety
    switch(insn->id) {
        case X86_INS_MOV:
            // Default case for MOV without special handling
            generate_mov_reg_imm(b, insn);
            break;
        case X86_INS_ADD:
        case X86_INS_SUB:
        case X86_INS_AND:
        case X86_INS_OR:
        case X86_INS_XOR:
            // Handle arithmetic with possible null bytes
            if (has_null_bytes(insn)) {
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                cs_x86_op *src_op = &insn->detail->x86.operands[1];
                
                if (src_op->type == X86_OP_IMM) {
                    uint32_t imm = (uint32_t)src_op->imm;
                    uint8_t dst_reg = dst_op->reg;
                    
                    // Save the destination register
                    uint8_t push_code[] = {0x50 + get_reg_index(dst_reg)}; // PUSH dst_reg
                    buffer_append(b, push_code, 1);

                    // Load the immediate value to EAX using null-safe method
                    generate_mov_eax_imm(b, imm);

                    // Perform the operation: op dst_reg, EAX
                    uint8_t op_code = 0x01; // Default to ADD
                    switch(insn->id) {
                        case X86_INS_ADD: op_code = 0x01; break;  // ADD
                        case X86_INS_SUB: op_code = 0x29; break;  // SUB
                        case X86_INS_AND: op_code = 0x21; break;  // AND
                        case X86_INS_OR:  op_code = 0x09; break;  // OR
                        case X86_INS_XOR: op_code = 0x31; break;  // XOR
                    }

                    uint8_t op_instr[] = {op_code, 0xC0}; // op reg, EAX
                    op_instr[1] = op_instr[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, op_instr, 2);

                    // Restore the destination register
                    uint8_t pop_code[] = {0x58 + get_reg_index(dst_reg)}; // POP dst_reg
                    buffer_append(b, pop_code, 1);
                } else {
                    // Non-immediate operand, use standard generation
                    generate_op_reg_imm(b, insn);
                }
            } else {
                generate_op_reg_imm(b, insn);
            }
            break;
        default:
            // For other instruction types, handle appropriately
            // Check if the instruction contains null bytes
            if (has_null_bytes(insn)) {
                // This is a problem case - we need to handle instructions with null bytes
                // that are not explicitly covered by our special cases
                buffer_append(b, insn->bytes, insn->size);
            } else {
                // No nulls in original instruction, append as-is
                buffer_append(b, insn->bytes, insn->size);
            }
            break;
    }

    // Verify that no null bytes were introduced by this strategy
    for (size_t i = initial_size; i < b->size; i++) {
        if (b->data[i] == 0x00) {
            fprintf(stderr, "ERROR: Multi-stage PEB strategy introduced null at offset %zu (relative offset %zu) in instruction: %s %s\n",
                   i, i - initial_size, insn->mnemonic, insn->op_str);
        }
    }
}

// Define the strategy structure
strategy_t multi_stage_peb_traversal_strategy = {
    .name = "Multi-Stage PEB Traversal Strategy",
    .can_handle = can_handle_multi_stage_peb_traversal,
    .get_size = get_size_multi_stage_peb_traversal,
    .generate = generate_multi_stage_peb_traversal_null_free,
    .priority = 97  // Critical priority for multi-stage PEB strategy
};

// Function to register multi-stage PEB traversal strategies
void register_multi_stage_peb_traversal_strategies() {
    register_strategy(&multi_stage_peb_traversal_strategy);
}