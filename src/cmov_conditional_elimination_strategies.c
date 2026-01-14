/*
 * CMOV Conditional Move Elimination Strategies
 *
 * PROBLEM: CMOV instructions (CMOVcc family) often contain bad bytes in:
 * - ModR/M bytes (0F 44 xx patterns can contain nulls)
 * - Displacement bytes when accessing memory with bad byte offsets
 * - CMOV is common in modern shellcode for anti-debugging and branchless logic
 *
 * SOLUTION: Replace CMOV with equivalent logic using SETcc + arithmetic operations
 * to maintain branchless execution semantics while avoiding bad bytes.
 */

#include "cmov_conditional_elimination_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Transform CMOV instruction to SETcc + arithmetic blending
 *
 * Technique 1: SETcc + Conditional Multiplication
 * Original: cmp eax, ebx; cmovz ecx, edx  (may contain bad chars in CMOV encoding)
 * Transform: cmp eax, ebx                 (compare as before)
 *            setz al                       (AL = 1 if zero, 0 otherwise)
 *            movzx eax, al                 (EAX = 0 or 1)
 *            dec eax                       (EAX = -1 (0xFFFFFFFF) or 0)
 *            mov esi, ecx                  (save original ECX)
 *            mov edi, edx                  (save EDX)
 *            and esi, eax                  (if zero: ESI=0, else: ESI=ECX)
 *            not eax                       (EAX = 0 or -1 (inverted))
 *            and edi, eax                  (if zero: EDI=EDX, else: EDI=0)
 *            or ecx, edi                   (ECX = EDX if zero, ECX if not zero)
 */
int transform_cmov_to_setcc_logic(cs_insn *insn, struct buffer *b) {
    if (!insn || !b) {
        return 0; // Error
    }

    // Check if instruction is a CMOV variant
    bool is_cmov = false;
    int condition_code = 0;  // Will store which condition (Z, NZ, G, L, etc.)

    switch(insn->id) {
        case X86_INS_CMOVA:   condition_code = 0; is_cmov = true; break;
        case X86_INS_CMOVAE:  condition_code = 1; is_cmov = true; break;
        case X86_INS_CMOVB:   condition_code = 2; is_cmov = true; break;
        case X86_INS_CMOVBE:  condition_code = 3; is_cmov = true; break;
        case X86_INS_CMOVE:   condition_code = 4; is_cmov = true; break;  // CMOVZ
        case X86_INS_CMOVG:   condition_code = 5; is_cmov = true; break;
        case X86_INS_CMOVGE:  condition_code = 6; is_cmov = true; break;
        case X86_INS_CMOVL:   condition_code = 7; is_cmov = true; break;
        case X86_INS_CMOVLE:  condition_code = 8; is_cmov = true; break;
        case X86_INS_CMOVNE:  condition_code = 9; is_cmov = true; break;  // CMOVNZ
        case X86_INS_CMOVNO:  condition_code = 10; is_cmov = true; break;
        case X86_INS_CMOVNP:  condition_code = 11; is_cmov = true; break;
        case X86_INS_CMOVNS:  condition_code = 12; is_cmov = true; break;
        case X86_INS_CMOVO:   condition_code = 13; is_cmov = true; break;
        case X86_INS_CMOVP:   condition_code = 14; is_cmov = true; break;
        case X86_INS_CMOVS:   condition_code = 15; is_cmov = true; break;
        default: is_cmov = false; break;
    }

    if (!is_cmov) {
        return 0; // Not a CMOV instruction
    }

    // Extract operands
    if (insn->detail->x86.op_count < 2) {
        return 0; // Need at least 2 operands for CMOV
    }

    cs_x86_op *dest_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    if (dest_op->type != X86_OP_REG || src_op->type != X86_OP_REG) {
        return 0; // Only handle register-to-register CMOV for now
    }

    int dest_reg = dest_op->reg;
    int src_reg = src_op->reg;

    // Determine which SETcc instruction corresponds to our condition
    uint8_t setcc_opcode = 0x90; // NOP as default
    switch(condition_code) {
        case 0:  setcc_opcode = 0x97; break;  // SETA
        case 1:  setcc_opcode = 0x93; break;  // SETAE
        case 2:  setcc_opcode = 0x92; break;  // SETB
        case 3:  setcc_opcode = 0x96; break;  // SETBE
        case 4:  setcc_opcode = 0x94; break;  // SETE/SETZ
        case 5:  setcc_opcode = 0x9F; break;  // SETG
        case 6:  setcc_opcode = 0x9D; break;  // SETGE
        case 7:  setcc_opcode = 0x9C; break;  // SETL
        case 8:  setcc_opcode = 0x9E; break;  // SETLE
        case 9:  setcc_opcode = 0x95; break;  // SETNE/SETNZ
        case 10: setcc_opcode = 0x91; break;  // SETNO
        case 11: setcc_opcode = 0x9B; break;  // SETNP
        case 12: setcc_opcode = 0x99; break;  // SETNS
        case 13: setcc_opcode = 0x90; break;  // SETO
        case 14: setcc_opcode = 0x9A; break;  // SETP
        case 15: setcc_opcode = 0x98; break;  // SETS
    }

    // Select temporary registers (avoid conflicts with operands)
    int temp_reg1 = X86_REG_ECX;
    int temp_reg2 = X86_REG_EDX;
    int temp_reg3 = X86_REG_ESI;
    (void)temp_reg3; // Suppress unused variable warning - may be used later
    // Use AL for SETcc result - not stored in variable since it's implicit

    // Check for conflicts and adjust if needed
    if (dest_reg == temp_reg1 || src_reg == temp_reg1) temp_reg1 = X86_REG_R8;
    if (dest_reg == temp_reg2 || src_reg == temp_reg2) temp_reg2 = X86_REG_R9;
    // temp_reg3 not currently used in this implementation but kept for future expansion

    // Step 1: SETcc to get condition result in AL
    // SETcc AL (condition result: 1 if true, 0 if false)
    buffer_write_byte(b, 0x0F);  // 2-byte opcode prefix
    buffer_write_byte(b, setcc_opcode);  // SETcc opcode
    buffer_write_byte(b, 0xC0);  // MOD/RM for AL register (same as MOV AL, AL)

    // Step 2: Zero-extend AL to full register size
    // MOVZX EAX, AL
    buffer_write_byte(b, 0x0F);  // 2-byte opcode prefix
    buffer_write_byte(b, 0xB6);  // MOVZX opcode
    buffer_write_byte(b, 0xC0);  // MOD/RM for EAX <- AL

    // Step 3: Convert 0/1 to 0xFFFFFFFF/0x00000000 (for masking)
    // NEG EAX (makes 1 become 0xFFFFFFFF, 0 stays 0)
    buffer_write_byte(b, 0xF7);  // NEG opcode
    buffer_write_byte(b, 0xD8);  // MOD/RM for EAX

    // Step 4: Save original destination value
    // MOV temp_reg1, dest_reg
    buffer_write_byte(b, 0x89);  // MOV reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(temp_reg1) << 3) | get_reg_index(dest_reg));  // MOD/RM

    // Step 5: Save source value
    // MOV temp_reg2, src_reg
    buffer_write_byte(b, 0x89);  // MOV reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(temp_reg2) << 3) | get_reg_index(src_reg));  // MOD/RM

    // Step 6: Mask original value with inverse condition
    // AND temp_reg1, EAX (this zeros the original if condition was true)
    buffer_write_byte(b, 0x21);  // AND reg, reg (reg is source, affects dest)
    buffer_write_byte(b, 0xC0 | (get_reg_index(temp_reg1) << 3) | get_reg_index(temp_reg1));  // MOD/RM

    // Step 7: Invert the mask for the source value
    // NOT EAX
    buffer_write_byte(b, 0xF7);  // NOT opcode
    buffer_write_byte(b, 0xD0);  // MOD/RM for EAX

    // Step 8: Mask source value with condition
    // AND temp_reg2, EAX (this zeros the source if condition was false)
    buffer_write_byte(b, 0x21);  // AND reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(temp_reg2) << 3) | get_reg_index(temp_reg2));  // MOD/RM

    // Step 9: Combine results
    // OR dest_reg, temp_reg1 (start with masked original)
    buffer_write_byte(b, 0x09);  // OR reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(dest_reg) << 3) | get_reg_index(temp_reg1));  // MOD/RM

    // OR dest_reg, temp_reg2 (add masked source)
    buffer_write_byte(b, 0x09);  // OR reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(dest_reg) << 3) | get_reg_index(temp_reg2));  // MOD/RM

    return 1; // Success
}

/**
 * Transform CMOV using XOR-based selection (alternative approach)
 *
 * Technique 2: XOR-Based Selection
 * Original: cmp eax, ebx; cmovz ecx, edx
 * Transform: cmp eax, ebx (as before)
 *            setz al        (AL = 1 if equal, 0 otherwise)
 *            movzx eax, al  (EAX = 0 or 1)
 *            neg eax        (EAX = 0 or 0xFFFFFFFF)
 *            xor ecx, edx   (temporary difference in ECX)
 *            and ecx, eax   (mask difference: 0 if not equal, original diff if equal)
 *            xor ecx, edx   (restore ECX to EDX if equal, or back to original if not)
 */
int transform_cmov_with_xor_logic(cs_insn *insn, struct buffer *b) {
    if (!insn || !b) {
        return 0; // Error
    }

    // Check if instruction is a CMOV variant
    bool is_cmov = false;
    int condition_code = 0;

    switch(insn->id) {
        case X86_INS_CMOVA:   condition_code = 0; is_cmov = true; break;
        case X86_INS_CMOVAE:  condition_code = 1; is_cmov = true; break;
        case X86_INS_CMOVB:   condition_code = 2; is_cmov = true; break;
        case X86_INS_CMOVBE:  condition_code = 3; is_cmov = true; break;
        case X86_INS_CMOVE:   condition_code = 4; is_cmov = true; break;  // CMOVZ
        case X86_INS_CMOVG:   condition_code = 5; is_cmov = true; break;
        case X86_INS_CMOVGE:  condition_code = 6; is_cmov = true; break;
        case X86_INS_CMOVL:   condition_code = 7; is_cmov = true; break;
        case X86_INS_CMOVLE:  condition_code = 8; is_cmov = true; break;
        case X86_INS_CMOVNE:  condition_code = 9; is_cmov = true; break;  // CMOVNZ
        case X86_INS_CMOVNO:  condition_code = 10; is_cmov = true; break;
        case X86_INS_CMOVNP:  condition_code = 11; is_cmov = true; break;
        case X86_INS_CMOVNS:  condition_code = 12; is_cmov = true; break;
        case X86_INS_CMOVO:   condition_code = 13; is_cmov = true; break;
        case X86_INS_CMOVP:   condition_code = 14; is_cmov = true; break;
        case X86_INS_CMOVS:   condition_code = 15; is_cmov = true; break;
        default: is_cmov = false; break;
    }

    if (!is_cmov) {
        return 0; // Not a CMOV instruction
    }

    // Extract operands
    if (insn->detail->x86.op_count < 2) {
        return 0; // Need at least 2 operands for CMOV
    }

    cs_x86_op *dest_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    if (dest_op->type != X86_OP_REG || src_op->type != X86_OP_REG) {
        return 0; // Only handle register-to-register CMOV for now
    }

    int dest_reg = dest_op->reg;
    int src_reg = src_op->reg;

    // Determine which SETcc instruction corresponds to our condition
    uint8_t setcc_opcode = 0x90; // NOP as default
    switch(condition_code) {
        case 0:  setcc_opcode = 0x97; break;  // SETA
        case 1:  setcc_opcode = 0x93; break;  // SETAE
        case 2:  setcc_opcode = 0x92; break;  // SETB
        case 3:  setcc_opcode = 0x96; break;  // SETBE
        case 4:  setcc_opcode = 0x94; break;  // SETE/SETZ
        case 5:  setcc_opcode = 0x9F; break;  // SETG
        case 6:  setcc_opcode = 0x9D; break;  // SETGE
        case 7:  setcc_opcode = 0x9C; break;  // SETL
        case 8:  setcc_opcode = 0x9E; break;  // SETLE
        case 9:  setcc_opcode = 0x95; break;  // SETNE/SETNZ
        case 10: setcc_opcode = 0x91; break;  // SETNO
        case 11: setcc_opcode = 0x9B; break;  // SETNP
        case 12: setcc_opcode = 0x99; break;  // SETNS
        case 13: setcc_opcode = 0x90; break;  // SETO
        case 14: setcc_opcode = 0x9A; break;  // SETP
        case 15: setcc_opcode = 0x98; break;  // SETS
    }

    // Select temporary register (avoid conflict with operands)
    int temp_reg = X86_REG_ECX;
    if (dest_reg == temp_reg || src_reg == temp_reg) temp_reg = X86_REG_EDX;
    int flag_reg = X86_REG_EAX;  // Use EAX for condition mask

    // Step 1: SETcc to get condition result in AL
    // SETcc AL
    buffer_write_byte(b, 0x0F);  // 2-byte opcode prefix
    buffer_write_byte(b, setcc_opcode);  // SETcc opcode
    buffer_write_byte(b, 0xC0);  // MOD/RM for AL register

    // Step 2: Zero-extend AL to full register size
    // MOVZX EAX, AL
    buffer_write_byte(b, 0x0F);  // 2-byte opcode prefix
    buffer_write_byte(b, 0xB6);  // MOVZX opcode
    buffer_write_byte(b, 0xC0);  // MOD/RM for EAX <- AL

    // Step 3: Convert 0/1 to 0x00000000/0xFFFFFFFF
    // NEG EAX (makes 1 become 0xFFFFFFFF, 0 stays 0)
    buffer_write_byte(b, 0xF7);  // NEG opcode
    buffer_write_byte(b, 0xD8);  // MOD/RM for EAX

    // Step 4: Calculate difference
    // XOR dest_reg, src_reg (store difference temporarily in dest_reg)
    buffer_write_byte(b, 0x31);  // XOR reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(dest_reg) << 3) | get_reg_index(src_reg));  // MOD/RM

    // Step 5: Mask the difference based on condition
    // AND dest_reg, EAX
    buffer_write_byte(b, 0x21);  // AND reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(dest_reg) << 3) | get_reg_index(flag_reg));  // MOD/RM

    // Step 6: Restore the result
    // XOR dest_reg, src_reg (restore to original value if condition was false,
    // or to src_reg value if condition was true)
    buffer_write_byte(b, 0x31);  // XOR reg, reg
    buffer_write_byte(b, 0xC0 | (get_reg_index(dest_reg) << 3) | get_reg_index(src_reg));  // MOD/RM

    return 1; // Success
}

/**
 * Main strategy application function
 */
void apply_cmov_elimination_strategy(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Try the SETcc + arithmetic blending approach first
    if (transform_cmov_to_setcc_logic(insn, b)) {
        return;
    }

    // Fall back to XOR-based approach
    transform_cmov_with_xor_logic(insn, b);
}

// Define the strategy structure as an external symbol for the registry
strategy_t cmov_conditional_elimination_strategy = {
    .name = "CMOV Conditional Move Elimination",
    .can_handle = can_handle_cmov_elimination_strategy,
    .get_size = get_size_cmov_elimination_strategy,
    .generate = apply_cmov_elimination_strategy,
    .priority = 92,
    .target_arch = BYVAL_ARCH_X86
};

// Helper functions for the strategy interface
int can_handle_cmov_elimination_strategy(cs_insn *insn) {
    // Check if instruction is a CMOV variant
    switch(insn->id) {
        case X86_INS_CMOVA:
        case X86_INS_CMOVAE:
        case X86_INS_CMOVB:
        case X86_INS_CMOVBE:
        case X86_INS_CMOVE:
        case X86_INS_CMOVG:
        case X86_INS_CMOVGE:
        case X86_INS_CMOVL:
        case X86_INS_CMOVLE:
        case X86_INS_CMOVNE:
        case X86_INS_CMOVNO:
        case X86_INS_CMOVNP:
        case X86_INS_CMOVNS:
        case X86_INS_CMOVO:
        case X86_INS_CMOVP:
        case X86_INS_CMOVS:
            // Check if the CMOV instruction has bad bytes in its encoding
            // For now, we'll handle all CMOV instructions as they can potentially have bad chars in ModR/M
            return 1;
        default:
            return 0; // Not a CMOV instruction
    }
}

size_t get_size_cmov_elimination_strategy(cs_insn *insn) {
    // Conservative estimate: transformed CMOV code will be 8-15 bytes vs 3-6 original
    (void)insn; // Suppress unused parameter warning
    return 15;
}