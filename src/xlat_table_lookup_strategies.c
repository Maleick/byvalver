/*
 * XLAT Table Lookup Strategy for Bad Character Elimination
 *
 * PROBLEM: XLAT instruction is commonly used in shellcode for byte translation
 * but the table address may contain bad bytes in displacement bytes.
 *
 * SOLUTION: Replace XLAT with equivalent logic using MOV from memory with
 * alternative addressing modes that avoid bad bytes.
 */

#include "xlat_table_lookup_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Transform XLAT instruction to MOV with alternative addressing
 *
 * Original: XLATB (translates AL using table at EBX+AL)
 * Transform: MOV AL, [EBX + EAX] (with EAX set to AL)
 */
int can_handle_xlat_to_mov_substitution(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if this is an XLAT instruction (X86_INS_XLATB in Capstone)
    if (insn->id == X86_INS_XLATB) {
        return 1;
    }

    return 0;
}

size_t get_size_xlat_to_mov_substitution(__attribute__((unused)) cs_insn *insn) {
    // MOV AL, [EBX+EAX] = 3 bytes, but we need additional setup
    // MOV EAX, 000000xx (to set up index) = 5 bytes
    // So total = 8 bytes vs original XLAT = 1 byte
    return 8;
}

void generate_xlat_to_mov_substitution(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }

    // Check if this is an XLAT instruction (X86_INS_XLATB in Capstone)
    if (insn->id != X86_INS_XLATB) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    // Alternative approach: preserve XLAT semantics but use different addressing
    // The XLAT instruction does: AL = [EBX + AL]
    // We'll use: MOVZX EAX, AL; ADD EAX, EBX; MOV AL, [EAX]

    // Step 1: Zero-extend AL to EAX to use as index
    // MOVZX EAX, AL
    buffer_write_byte(b, 0x0F);
    buffer_write_byte(b, 0xB6);
    buffer_write_byte(b, 0xC0);  // MOD/RM for EAX <- AL

    // Step 2: Add base address (EBX) to index (EAX)
    // ADD EAX, EBX
    buffer_write_byte(b, 0x01);
    buffer_write_byte(b, 0xD8);  // MOD/RM for EBX <- EAX (reversed)

    // Step 3: Load the byte from the calculated address
    // MOV AL, [EAX]
    buffer_write_byte(b, 0x8A);
    buffer_write_byte(b, 0x00);  // MOD/RM for AL <- [EAX]
}

/**
 * Transform XLAT with table address that contains bad bytes
 *
 * Original: XLATB with table at address containing bad chars
 * Transform: Create new table without bad chars or use alternative approach
 */
int can_handle_xlat_bad_byte_table(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if this is an XLAT instruction (X86_INS_XLATB in Capstone)
    if (insn->id == X86_INS_XLATB) {
        // For now, we'll handle all XLAT instructions
        // In the future, we could check if the table address contains bad chars
        return 1;
    }

    return 0;
}

size_t get_size_xlat_bad_byte_table(__attribute__((unused)) cs_insn *insn) {
    // Size for alternative implementation
    return 10;  // Conservative estimate
}

void generate_xlat_bad_byte_table(struct buffer *b, cs_insn *insn) {
    // For this implementation, we'll use the same approach as above
    generate_xlat_to_mov_substitution(b, insn);
}

// Define the strategy structure
strategy_t xlat_table_lookup_strategy = {
    .name = "XLAT Table Lookup Elimination",
    .can_handle = can_handle_xlat_bad_byte_table,
    .get_size = get_size_xlat_bad_byte_table,
    .generate = generate_xlat_bad_byte_table,
    .priority = 72  // As specified in documentation
};