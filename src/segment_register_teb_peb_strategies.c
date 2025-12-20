/*
 * Segment Register TEB/PEB Access Strategies
 *
 * PROBLEM: Direct access to TEB/PEB via FS:[offset] or GS:[offset] often involves
 * immediate values that contain null bytes or other bad characters:
 * - mov eax, fs:[0x30] → 64 A1 30 00 00 00 (contains 3 null bytes)
 * - mov eax, fs:[0x0]  → 64 A1 00 00 00 00 (all null bytes)
 * - mov ebx, gs:[0x68] → 65 8B 1D 68 00 00 00 (contains 3 null bytes)
 *
 * SOLUTION: Use segment register access with null-byte-free offsets and alternative
 * addressing modes to access TEB/PEB fields without bad characters.
 */

#include "segment_register_teb_peb_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Common TEB/PEB offsets that often contain bad characters
#define TEB_SELF_POINTER_OFFSET      0x00  // FS:[0x0] - Self pointer
#define TEB_EXCEPTION_LIST_OFFSET    0x00  // FS:[0x0] - Exception list
#define TEB_STACK_BASE_OFFSET        0x04  // FS:[0x4] - Stack base
#define TEB_STACK_LIMIT_OFFSET       0x08  // FS:[0x8] - Stack limit
#define TEB_PEB_POINTER_OFFSET       0x30  // FS:[0x30] - PEB pointer (common in shellcode)
#define TEB_LAST_ERROR_OFFSET        0x34  // FS:[0x34] - Last error value
#define TEB_COUNT_OF_EXCEPTIONS      0x1A4 // FS:[0x1A4] - Count of exceptions

// PEB offsets accessed via GS on x64
#define PEB_SELF_POINTER_OFFSET      0x00  // GS:[0x0] on x64
#define PEB_IMAGE_BASE_OFFSET        0x10  // GS:[0x10] on x64 - Image base address
#define PEB_LDR_POINTER_OFFSET       0x18  // GS:[0x18] on x64 - Loader data pointer
#define PEB_PROCESS_PARAMETERS       0x20  // GS:[0x20] on x64 - Process parameters

/**
 * Transform FS/GS segment register access to avoid bad characters in offsets
 *
 * Technique 1: Use register indirect addressing instead of immediate offset
 * Original: mov eax, fs:[0x30]     (64 A1 30 00 00 00 - contains nulls)
 * Transform: xor eax, eax          (clear EAX)
 *            mov al, 0x30          (load offset 0x30 into AL - no nulls)
 *            mov ebx, fs:[eax]     (access FS:[EAX] - no immediate offset)
 *
 * Technique 2: Use LEA for offset calculation
 * Original: mov eax, fs:[0x300]    (large offset may contain bad chars)
 * Transform: lea eax, [fs:0x200]   (calculate part of offset)
 *            add eax, 0x100        (add remainder without bad chars)
 */
int transform_fs_gs_access(cs_insn *insn, struct buffer *b) {
    if (!insn || !b) {
        return 0; // Error
    }

    // Check if instruction uses segment override (FS: or GS:)
    if (insn->id != X86_INS_MOV) {
        return 0; // Not a MOV instruction
    }

    // Check for segment override in operands
    bool has_segment_override = false;
    int segment_reg = 0;
    int32_t offset = 0;

    // Check if any operand has a segment override
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        cs_x86_op *op = &insn->detail->x86.operands[i];
        if (op->type == X86_OP_MEM && op->mem.segment != X86_REG_INVALID) {
            segment_reg = op->mem.segment;
            offset = op->mem.disp;
            has_segment_override = true;
            break;
        }
    }

    if (!has_segment_override) {
        return 0; // No segment override found
    }

    // Check if offset contains bad characters
    bool offset_has_bad_chars = false;
    uint8_t *offset_bytes = (uint8_t*)&offset;
    for (int i = 0; i < 4; i++) {
        if (offset_bytes[i] == 0x00) { // Check for null bytes specifically
            offset_has_bad_chars = true;
            break;
        }
    }

    if (!offset_has_bad_chars) {
        return 0; // No bad characters in offset
    }

    // Choose a temporary register for the offset
    x86_reg temp_reg = X86_REG_EBX; // Use EBX as temporary register
    if (insn->detail->x86.operands[0].type == X86_OP_REG &&
        insn->detail->x86.operands[0].reg == temp_reg) {
        temp_reg = X86_REG_EDX; // Use EDX if EBX is destination
    }

    // Method 1: Load offset into register and use indirect addressing
    // Clear the temporary register
    // XOR temp_reg, temp_reg (to clear register)
    buffer_write_byte(b, 0x33);  // XOR opcode
    buffer_write_byte(b, 0xC0 + get_reg_index(temp_reg));  // MOD/RM for register XOR

    // Load offset byte by byte to avoid bad characters
    uint32_t remaining_offset = (uint32_t)offset;
    int byte_idx = 0;

    // Handle each byte of the offset
    while (remaining_offset != 0) {
        uint8_t current_byte = remaining_offset & 0xFF;
        if (current_byte != 0x00) { // Skip null bytes
            // MOV temp_reg_low, current_byte (for 8-bit register)
            buffer_write_byte(b, 0xB0 + get_reg_index(temp_reg)); // MOV AL, imm8
            buffer_write_byte(b, current_byte);

            // If this isn't the lowest byte, shift left by 8*byte_idx
            if (byte_idx > 0) {
                // SHL temp_reg, 8*byte_idx (shift by byte position)
                for (int shift = 0; shift < byte_idx; shift++) {
                    buffer_write_byte(b, 0xC1);  // SHL opcode
                    buffer_write_byte(b, 0xE0 + get_reg_index(temp_reg));  // MOD/RM for register
                    buffer_write_byte(b, 0x08);  // Shift by 8 (one byte)
                }
            }
        }
        remaining_offset >>= 8;
        byte_idx++;
    }

    // Now perform the segment register access using indirect addressing
    // MOV dest_reg, [segment:temp_reg]
    cs_x86_op *dest_op = &insn->detail->x86.operands[0];
    cs_x86_op *mem_op = &insn->detail->x86.operands[1];

    if (mem_op->type != X86_OP_MEM) {
        mem_op = &insn->detail->x86.operands[0];
        dest_op = &insn->detail->x86.operands[1];
    }

    if (dest_op->type == X86_OP_REG && mem_op->type == X86_OP_MEM) {
        int dest_reg = dest_op->reg;

        // Add segment prefix (FS=0x64, GS=0x65)
        if (segment_reg == X86_REG_FS) {
            buffer_write_byte(b, 0x64);
        } else if (segment_reg == X86_REG_GS) {
            buffer_write_byte(b, 0x65);
        }

        // MOV instruction with indirect addressing
        buffer_write_byte(b, 0x8B);  // MOV reg, [reg]

        // MOD/RM byte: 00 reg reg (indirect addressing)
        buffer_write_byte(b, (0x00 << 6) | (get_reg_index(dest_reg) << 3) | get_reg_index(temp_reg));
    }

    return 1; // Success
}

/**
 * Transform direct TEB/PEB access patterns to avoid bad characters
 * Common pattern: mov eax, fs:[0x30] (access PEB pointer)
 */
int transform_teb_peb_access(cs_insn *insn, struct buffer *b) {
    if (!insn || !b) {
        return 0; // Error
    }

    // Check if this is a common TEB/PEB access pattern
    if (insn->id != X86_INS_MOV) {
        return 0; // Not a MOV instruction
    }

    // Look for MOV reg, [segment:immediate_offset] pattern
    bool found_teb_peb_pattern = false;
    int32_t offset = 0;
    int segment_reg = 0;

    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        cs_x86_op *op = &insn->detail->x86.operands[i];
        if (op->type == X86_OP_MEM && op->mem.segment != X86_REG_INVALID) {
            segment_reg = op->mem.segment;
            offset = op->mem.disp;

            // Check if this is a common TEB/PEB offset that might have bad chars
            if ((segment_reg == X86_REG_FS && (offset == 0x30 || offset == 0x0 || offset == 0x34)) ||
                (segment_reg == X86_REG_GS && (offset == 0x10 || offset == 0x18 || offset == 0x20))) {
                found_teb_peb_pattern = true;
                break;
            }
        }
    }

    if (!found_teb_peb_pattern) {
        return 0; // Not a recognized TEB/PEB access pattern
    }

    // Apply the same transformation as transform_fs_gs_access
    return transform_fs_gs_access(insn, b);
}

/**
 * Main strategy application function
 */
void apply_segment_register_strategy(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Try different transformation methods in order of preference

    // Method 1: Transform FS/GS access
    if (transform_fs_gs_access(insn, b)) {
        return;
    }

    // Method 2: Transform TEB/PEB access patterns
    transform_teb_peb_access(insn, b);
}

// Define the strategy structure as an external symbol for the registry
strategy_t segment_register_teb_peb_strategy = {
    .name = "Segment Register TEB/PEB Access",
    .can_handle = can_handle_segment_register_strategy,
    .get_size = get_size_segment_register_strategy,
    .generate = apply_segment_register_strategy,
    .priority = 94
};

// Helper functions for the strategy interface
int can_handle_segment_register_strategy(cs_insn *insn) {
    // Check if any operand uses segment override (FS: or GS:)
    if (insn->id == X86_INS_MOV) {
        for (int i = 0; i < insn->detail->x86.op_count; i++) {
            cs_x86_op *op = &insn->detail->x86.operands[i];
            if (op->type == X86_OP_MEM && op->mem.segment != X86_REG_INVALID) {
                // Check if displacement contains bad characters
                int32_t offset = op->mem.disp;
                uint8_t *offset_bytes = (uint8_t*)&offset;
                for (int j = 0; j < 4; j++) {
                    if (offset_bytes[j] == 0x00) { // Check for null bytes
                        return 1; // Can handle this instruction
                    }
                }
            }
        }
    }
    return 0; // Cannot handle this instruction
}

size_t get_size_segment_register_strategy(cs_insn *insn) {
    // Conservative estimate: transformed code will be 5-10 bytes
    (void)insn; // Suppress unused parameter warning
    return 10;
}