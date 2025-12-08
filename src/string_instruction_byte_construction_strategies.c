/*
 * String Instruction Byte Construction Strategy
 *
 * PROBLEM: Constructing values containing null bytes (or immediate values with null bytes)
 * can lead to null bytes in the instruction encoding.
 *
 * SOLUTION: Use STOSx (STOSB, STOSD) and related string instructions with loops
 * to construct values containing nulls byte-by-byte in memory, avoiding direct
 * immediate encoding with null bytes.
 *
 * FREQUENCY: Useful when constructing complex values or initializing memory areas
 * PRIORITY: 82 (High - important for memory initialization and complex constant construction)
 *
 * Example transformations:
 *   Original: MOV EAX, 0x00123456 (contains null in first byte)
 *   Strategy: Use ECX counter + STOSD loop to build value byte by byte without null immediates
 */

#include "string_instruction_byte_construction_strategies.h"
#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*
 * Detection function for MOV operations with immediate values containing null bytes
 */
int can_handle_string_instruction_byte_construction(cs_insn *insn) {
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

    // Check if the immediate contains null bytes
    uint32_t imm = (uint32_t)src_op->imm;
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            // Check if the original instruction encoding contains nulls too
            for (size_t j = 0; j < insn->size; j++) {
                if (insn->bytes[j] == 0x00) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*
 * Size calculation for string instruction byte construction
 *
 * This approach uses loops and multiple instructions, so it's larger than direct MOV
 * but avoids null bytes in immediate values
 */
size_t get_size_string_instruction_byte_construction(cs_insn *insn) {
    (void)insn; // Unused parameter

    // This will be multiple instructions: loop setup + string instructions + loop body
    return 20; // Conservative estimate
}

/*
 * Generate string instruction byte construction
 *
 * For MOV reg, imm32 that contains null bytes:
 *   PUSH the target register to save it
 *   MOV DI/EDI to point to temporary memory
 *   Build value byte by byte using STOSB/STOSD
 *   MOV result back to target register
 */
void generate_string_instruction_byte_construction(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t value_to_construct = (uint32_t)src_op->imm;
    
    // Use a temporary memory location - we'll use the stack
    // PUSH registers we'll use
    uint8_t push_ecx[] = {0x51};  // PUSH ECX
    uint8_t push_edi[] = {0x57};  // PUSH EDI
    buffer_append(b, push_ecx, 1);
    buffer_append(b, push_edi, 1);
    
    // Set up EDI to point to temporary location on stack
    uint8_t lea_edi_esp[] = {0x8D, 0x7C, 0x24, 0xF8};  // LEA EDI, [ESP-8] - safe location
    buffer_append(b, lea_edi_esp, 4);
    
    // We'll build the value byte by byte
    // For this example, we'll construct each byte separately
    uint8_t byte0 = (value_to_construct >> 0) & 0xFF;
    uint8_t byte1 = (value_to_construct >> 8) & 0xFF;
    uint8_t byte2 = (value_to_construct >> 16) & 0xFF;
    uint8_t byte3 = (value_to_construct >> 24) & 0xFF;
    
    // MOV AL, byte0 (constructed without null)
    generate_mov_eax_imm_byte(b, byte0);
    // STOSB to store AL to [EDI] and increment EDI
    uint8_t stosb[] = {0xAA};
    buffer_append(b, stosb, 1);
    
    // MOV AL, byte1
    generate_mov_eax_imm_byte(b, byte1);
    buffer_append(b, stosb, 1);
    
    // MOV AL, byte2
    generate_mov_eax_imm_byte(b, byte2);
    buffer_append(b, stosb, 1);
    
    // MOV AL, byte3
    generate_mov_eax_imm_byte(b, byte3);
    buffer_append(b, stosb, 1);
    
    // Now load the constructed value back into EAX
    uint8_t mov_eax_dword_ptr_edi[] = {0x8B, 0x07};  // MOV EAX, [EDI-4] (pointing back to our data)
    buffer_append(b, mov_eax_dword_ptr_edi, 2);
    // Adjust EDI back to start position
    uint8_t sub_edi_4[] = {0x83, 0xEF, 0x04};  // SUB EDI, 4
    buffer_append(b, sub_edi_4, 3);
    
    // Now move from EAX to the target register
    if (target_reg != X86_REG_EAX) {
        // MOV target_reg, EAX
        uint8_t mov_target_eax[] = {0x89, 0xC0};
        mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
        buffer_append(b, mov_target_eax, 2);
    }
    
    // Restore registers
    uint8_t pop_edi[] = {0x5F};  // POP EDI
    uint8_t pop_ecx[] = {0x59};  // POP ECX
    buffer_append(b, pop_edi, 1);
    buffer_append(b, pop_ecx, 1);
}

/*
 * Alternative approach: Use STOSD to store a complete 32-bit value
 */
int can_handle_stosd_construction(cs_insn *insn) {
    return can_handle_string_instruction_byte_construction(insn);
}

size_t get_size_stosd_construction(cs_insn *insn) {
    (void)insn;
    return 18;  // Smaller approach using STOSD
}

void generate_stosd_construction(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    
    x86_reg target_reg = dst_op->reg;
    uint32_t value_to_construct = (uint32_t)src_op->imm;
    
    // Save used registers
    uint8_t push_eax[] = {0x50};  // PUSH EAX
    uint8_t push_edi[] = {0x57};  // PUSH EDI
    buffer_append(b, push_eax, 1);
    buffer_append(b, push_edi, 1);
    
    // Set up EDI to point to temporary location
    uint8_t lea_edi_esp[] = {0x8D, 0x7C, 0x24, 0xF0};  // LEA EDI, [ESP-16] - safe location
    buffer_append(b, lea_edi_esp, 4);
    
    // Load the value to construct into EAX (using null-free construction)
    generate_mov_eax_imm(b, value_to_construct);
    
    // STOSD to store EAX to [EDI] and increment EDI
    uint8_t stosd[] = {0xAB};
    buffer_append(b, stosd, 1);
    
    // Move the value back to EDI to access it
    // MOV EAX, [EDI-4] to get the stored value
    uint8_t mov_eax_dword_ptr_edi_back[] = {0x8B, 0x47, 0xFC};  // MOV EAX, [EDI-4]
    buffer_append(b, mov_eax_dword_ptr_edi_back, 3);
    
    // Move from EAX to the target register
    if (target_reg != X86_REG_EAX) {
        uint8_t mov_target_eax[] = {0x89, 0xC0};
        mov_target_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
        buffer_append(b, mov_target_eax, 2);
    }
    
    // Restore registers
    uint8_t pop_edi[] = {0x5F};  // POP EDI
    uint8_t pop_eax[] = {0x58};  // POP EAX
    buffer_append(b, pop_edi, 1);
    buffer_append(b, pop_eax, 1);
}

strategy_t string_instruction_byte_construction_strategy = {
    .name = "String Instruction Byte Construction",
    .can_handle = can_handle_string_instruction_byte_construction,
    .get_size = get_size_string_instruction_byte_construction,
    .generate = generate_string_instruction_byte_construction,
    .priority = 82  // High priority
};

strategy_t stosd_construction_strategy = {
    .name = "STOSD Construction",
    .can_handle = can_handle_stosd_construction,
    .get_size = get_size_stosd_construction,
    .generate = generate_stosd_construction,
    .priority = 80  // High priority
};

void register_string_instruction_byte_construction_strategies() {
    register_strategy(&string_instruction_byte_construction_strategy);
    register_strategy(&stosd_construction_strategy);
}