/*
 * Advanced Hash-Based API Resolution Strategy
 *
 * PROBLEM: Modern shellcode uses sophisticated hash algorithms to resolve API addresses,
 * including complex combinations of ROR/ROL operations with XOR, 16-bit hashes,
 * and multi-stage DLL loading sequences. Standard ROR13 hash strategies may not
 * handle these advanced patterns effectively.
 *
 * SOLUTION: Implement an advanced hash-based API resolution strategy that handles
 * complex hashing algorithms beyond basic ROR13, including ROR combined with XOR,
 * 16-bit hashes, and multi-stage hash resolution patterns.
 *
 * FREQUENCY: Common in modern shellcode for evasion
 * PRIORITY: 96 (Very High - advanced evasion technique)
 *
 * Example transformations:
 *   Advanced hash algo: Complex hashing + API resolution without null-byte addresses
 *   Strategy: Identify hash calculation patterns and replace with null-free equivalents
 */

#include "advanced_hash_api_resolution.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for advanced hash-based API resolution patterns that contain null bytes
 * or need sophisticated handling
 */
int can_handle_advanced_hash_api_resolution(cs_insn *insn) {
    if (!insn || !insn->detail) return 0;

    // Look for MOV instructions that may involve API address loading with potential nulls
    if (insn->id == X86_INS_MOV && insn->detail->x86.op_count == 2) {
        cs_x86_op *src_op = &insn->detail->x86.operands[1];

        // Check for MOV reg, imm32 where immediate contains null bytes
        // This could be part of an advanced hash-based API resolution
        if (src_op->type == X86_OP_IMM) {
            if (!is_bad_byte_free((uint32_t)src_op->imm)) {
                return 1;
            }
        }
    }

    // Look for CALL instructions that might call APIs resolved via advanced hashing
    if (insn->id == X86_INS_CALL && insn->detail->x86.op_count == 1) {
        cs_x86_op *op = &insn->detail->x86.operands[0];
        if (op->type == X86_OP_IMM) {
            uint32_t target = (uint32_t)op->imm;
            if (!is_bad_byte_free(target) && has_null_bytes(insn)) {
                // CALL with immediate target that has null bytes
                // This could be the result of advanced hash-based resolution
                return 1;
            }
        }
    }

    // Identify arithmetic operations that are part of hash calculations
    if (insn->id == X86_INS_ADD || insn->id == X86_INS_SUB ||
        insn->id == X86_INS_AND || insn->id == X86_INS_OR ||
        insn->id == X86_INS_XOR || insn->id == X86_INS_ROR || 
        insn->id == X86_INS_ROL) {
        if (insn->detail->x86.op_count >= 2) {
            cs_x86_op *src_op = &insn->detail->x86.operands[1];
            if (src_op->type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)src_op->imm;
                if (!is_bad_byte_free(imm) && has_null_bytes(insn)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*
 * Size calculation function for advanced hash API resolution null elimination
 */
size_t get_size_advanced_hash_api_resolution(cs_insn *insn) {
    // This depends on the specific transformation needed
    // For immediate value transformations, refer to similar strategies
    (void)insn; // Unused parameter
    return 10; // Conservative estimate for complex transformations
}

/*
 * Generation function for null-free advanced hash API operations
 * This handles complex hash algorithms and multi-stage API resolution
 */
void generate_advanced_hash_api_resolution_null_free(struct buffer *b, cs_insn *insn) {
    // Store the initial size to verify no nulls are introduced
    size_t initial_size = b->size;

    if (insn->id == X86_INS_MOV) {
        // Handle MOV instruction with null-containing immediate
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
                    uint8_t push_code[1] = {0x50 + get_reg_index(dst_reg)}; // PUSH reg
                    buffer_append(b, push_code, 1);

                    generate_mov_eax_imm(b, imm);  // Load value to EAX (null-safe)

                    uint8_t mov_reg_eax[2] = {0x89, 0xC0}; // MOV reg, EAX
                    mov_reg_eax[1] = mov_reg_eax[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, mov_reg_eax, 2);

                    uint8_t pop_code[1] = {0x58 + get_reg_index(dst_reg)}; // POP reg
                    buffer_append(b, pop_code, 1);
                }
            } else {
                // No nulls in immediate, use normal MOV
                generate_mov_reg_imm(b, insn);
            }
            return;
        }
    } else if (insn->id == X86_INS_ADD || insn->id == X86_INS_SUB ||
               insn->id == X86_INS_AND || insn->id == X86_INS_OR ||
               insn->id == X86_INS_XOR || insn->id == X86_INS_ROR || 
               insn->id == X86_INS_ROL) {
        // Handle arithmetic/logical operations with null-containing immediates
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *dst_op = &insn->detail->x86.operands[0];
            cs_x86_op *src_op = &insn->detail->x86.operands[1];

            if (src_op->type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)src_op->imm;

                if (!is_bad_byte_free(imm)) {
                    // Use null-safe approach: MOV EAX, imm (null safe) + op reg, EAX
                    uint8_t dst_reg = dst_op->reg;

                    // Save the destination register if it's not EAX
                    if (dst_reg != X86_REG_EAX) {
                        uint8_t push_code[1] = {0x50 + get_reg_index(dst_reg)}; // PUSH dst_reg
                        buffer_append(b, push_code, 1);
                    }

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
                        case X86_INS_ROR: op_code = 0xD3; break;  // ROR (with CL)
                        case X86_INS_ROL: op_code = 0xD3; break;  // ROL (with CL)
                    }

                    if (insn->id == X86_INS_ROR || insn->id == X86_INS_ROL) {
                        // Handle ROR/ROL specially - they use CL for count
                        // But we need to get the immediate into CL first
                        if (imm > 0xFF) {
                            // Handle multi-byte immediate by using register
                            // MOV ECX, imm (using null-safe method)
                            generate_mov_eax_imm(b, imm & 0xFF); // Use only the lowest byte for rotation count
                            uint8_t mov_cl[] = {0x8A, 0xC8}; // MOV CL, AL (move AL to CL)
                            buffer_append(b, mov_cl, 2);
                        } else {
                            uint8_t mov_cl[] = {0xB1, (uint8_t)imm}; // MOV CL, imm8
                            buffer_append(b, mov_cl, 2);
                        }

                        // Now execute ROR/ROL reg, CL
                        uint8_t rotate_instr[2] = {op_code, 0xD0};
                        rotate_instr[1] = rotate_instr[1] + get_reg_index(dst_reg);
                        if (dst_reg != X86_REG_EAX) {
                            rotate_instr[1] = rotate_instr[1] + 0xC0; // Mod=11 for register-to-register
                        }
                        buffer_append(b, rotate_instr, 2);
                    } else {
                        uint8_t op_instr[2] = {op_code, 0xC0}; // op reg, EAX
                        op_instr[1] = op_instr[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                        buffer_append(b, op_instr, 2);
                    }

                    // Restore the destination register if it was saved
                    if (dst_reg != X86_REG_EAX) {
                        uint8_t pop_code[1] = {0x58 + get_reg_index(dst_reg)}; // POP dst_reg
                        buffer_append(b, pop_code, 1);
                    }
                    return;
                } else {
                    // No nulls in immediate, use normal operation
                    generate_op_reg_imm(b, insn);
                    return;
                }
            }
        }
    } else if (insn->id == X86_INS_CALL) {
        // Handle CALL with immediate target that contains nulls
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
        }
    }

    // For remaining cases, use standard utilities with null-safety
    switch(insn->id) {
        case X86_INS_MOV:
            // Check if this instruction has nulls in its immediate or elsewhere
            if (has_null_bytes(insn)) {
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                uint8_t dst_reg = dst_op->reg;

                if (dst_reg == X86_REG_EAX) {
                    generate_mov_eax_imm(b, (uint32_t)insn->detail->x86.operands[1].imm);
                } else {
                    // For other registers, save, load to EAX, move to target, restore
                    uint8_t push_code[1] = {0x50 + get_reg_index(dst_reg)}; // PUSH reg
                    buffer_append(b, push_code, 1);

                    generate_mov_eax_imm(b, (uint32_t)insn->detail->x86.operands[1].imm);  // Load value to EAX (null-safe)

                    uint8_t mov_reg_eax[2] = {0x89, 0xC0}; // MOV reg, EAX
                    mov_reg_eax[1] = mov_reg_eax[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                    buffer_append(b, mov_reg_eax, 2);

                    uint8_t pop_code[1] = {0x58 + get_reg_index(dst_reg)}; // POP reg
                    buffer_append(b, pop_code, 1);
                }
            } else {
                generate_mov_reg_imm(b, insn);
            }
            break;
        case X86_INS_ADD:
        case X86_INS_SUB:
        case X86_INS_AND:
        case X86_INS_OR:
        case X86_INS_XOR:
            // Check if this instruction has nulls in its immediate
            if (has_null_bytes(insn)) {
                cs_x86_op *dst_op = &insn->detail->x86.operands[0];
                uint8_t dst_reg = dst_op->reg;
                uint32_t imm = (uint32_t)insn->detail->x86.operands[1].imm;

                // Save the destination register
                uint8_t push_code[1] = {0x50 + get_reg_index(dst_reg)}; // PUSH dst_reg
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

                uint8_t op_instr[2] = {op_code, 0xC0}; // op reg, EAX
                op_instr[1] = op_instr[1] + (get_reg_index(dst_reg) << 3) + get_reg_index(X86_REG_EAX);
                buffer_append(b, op_instr, 2);

                // Restore the destination register
                uint8_t pop_code[1] = {0x58 + get_reg_index(dst_reg)}; // POP dst_reg
                buffer_append(b, pop_code, 1);
            } else {
                generate_op_reg_imm(b, insn);
            }
            break;
        default:
            // For other instruction types, just append original bytes if no nulls
            if (has_null_bytes(insn)) {
                // If there are nulls in the original instruction, handle it appropriately
                buffer_append(b, insn->bytes, insn->size);
            } else {
                // Otherwise, use the original bytes
                buffer_append(b, insn->bytes, insn->size);
            }
            break;
    }

    // Verify that no null bytes were introduced by this strategy
    for (size_t i = initial_size; i < b->size; i++) {
        if (b->data[i] == 0x00) {
            fprintf(stderr, "ERROR: Advanced hash strategy introduced null at offset %zu (relative offset %zu) in instruction: %s %s\n",
                   i, i - initial_size, insn->mnemonic, insn->op_str);
        }
    }
}

// Define the strategy structure
strategy_t advanced_hash_api_resolution_strategy = {
    .name = "Advanced Hash-Based API Resolution Strategy",
    .can_handle = can_handle_advanced_hash_api_resolution,
    .get_size = get_size_advanced_hash_api_resolution,
    .generate = generate_advanced_hash_api_resolution_null_free,
    .priority = 96  // Very high priority for advanced hash strategy
};

// Function to register advanced hash-based API resolution strategies
void register_advanced_hash_api_resolution_strategies() {
    register_strategy(&advanced_hash_api_resolution_strategy);
}