/*
 * LAHF/SAHF Flag Preservation Strategy for Bad Character Elimination
 *
 * PROBLEM: LAHF/SAHF instructions (Load/Store AH from/to flags) may contain
 * bad characters in their opcodes or may need to be replaced when working
 * with shellcode that has bad character restrictions.
 *
 * SOLUTION: Replace LAHF/SAHF with PUSHF/POPF or manual flag manipulation
 * that avoids bad characters.
 */

#include "lahf_sahf_flag_preservation_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Transform LAHF (Load Flags to AH) to alternative implementation
 *
 * Original: LAHF (loads SF, ZF, AF, PF, CF to AH)
 * Transform: PUSHF; POP EAX; MOV AH, AL (where AL contains the flags)
 */
int can_handle_lahf_alternative(cs_insn *insn) {
    if (!insn) {
        return 0;
    }
    
    // Check if this is a LAHF instruction
    if (insn->id == X86_INS_LAHF) {
        return 1;
    }
    
    return 0;
}

size_t get_size_lahf_alternative(__attribute__((unused)) cs_insn *insn) {
    // PUSHF (1 byte) + POP EAX (1 byte) + MOV AH, AL (2 bytes) = 4 bytes
    // vs original LAHF (1 byte)
    return 4;
}

void generate_lahf_alternative(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }
    
    // Check if this is a LAHF instruction
    if (insn->id != X86_INS_LAHF) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }
    
    // Transform LAHF to: PUSHF; POP EAX; MOV AH, AL
    // PUSHF - save flags to stack
    buffer_write_byte(b, 0x9C);  // PUSHF
    
    // POP EAX - get flags into EAX
    buffer_write_byte(b, 0x58);  // POP EAX
    
    // MOV AH, AL - move low byte of flags to AH
    buffer_write_byte(b, 0x88);  // MOV r8, r8
    buffer_write_byte(b, 0xE4);  // MOD/RM for AH <- AL
}

/**
 * Transform SAHF (Store AH to Flags) to alternative implementation
 *
 * Original: SAHF (loads SF, ZF, AF, PF, CF from AH)
 * Transform: PUSH EAX; PUSH EAX; POPF (manipulate flags via EAX)
 */
int can_handle_sahf_alternative(cs_insn *insn) {
    if (!insn) {
        return 0;
    }
    
    // Check if this is a SAHF instruction
    if (insn->id == X86_INS_SAHF) {
        return 1;
    }
    
    return 0;
}

size_t get_size_sahf_alternative(__attribute__((unused)) cs_insn *insn) {
    // PUSH EAX (1 byte) + PUSH EAX (1 byte) + POPF (1 byte) = 3 bytes
    // vs original SAHF (1 byte)
    return 3;
}

void generate_sahf_alternative(struct buffer *b, cs_insn *insn) {
    if (!insn || !b) {
        return;
    }
    
    // Check if this is a SAHF instruction
    if (insn->id != X86_INS_SAHF) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }
    
    // Transform SAHF to: PUSH EAX; PUSH EAX; POPF
    // This is a simplified approach - we need to manipulate the flags properly
    
    // First, we need to get the current flags, modify AH part, then restore
    // PUSHF; POP EBX; MOV BL, AH; PUSH EBX; POPF
    buffer_write_byte(b, 0x9C);  // PUSHF - save current flags
    buffer_write_byte(b, 0x5B);  // POP EBX - get flags to EBX
    buffer_write_byte(b, 0x88);  // MOV BL, AH - put AH into BL (low byte of flags)
    buffer_write_byte(b, 0xE3);  // MOD/RM for BL <- AH
    buffer_write_byte(b, 0x53);  // PUSH EBX - push modified flags
    buffer_write_byte(b, 0x9D);  // POPF - restore flags with AH values
}

/**
 * Complete strategy implementation that handles both LAHF and SAHF
 */
int can_handle_lahf_sahf_alternative(cs_insn *insn) {
    if (!insn) {
        return 0;
    }
    
    // Check if this is a LAHF or SAHF instruction
    if (insn->id == X86_INS_LAHF || insn->id == X86_INS_SAHF) {
        return 1;
    }
    
    return 0;
}

size_t get_size_lahf_sahf_alternative(cs_insn *insn) {
    if (insn->id == X86_INS_LAHF) {
        return get_size_lahf_alternative(insn);
    } else if (insn->id == X86_INS_SAHF) {
        return get_size_sahf_alternative(insn);
    }
    return 5;  // Conservative estimate
}

void generate_lahf_sahf_alternative(struct buffer *b, cs_insn *insn) {
    if (insn->id == X86_INS_LAHF) {
        generate_lahf_alternative(b, insn);
    } else if (insn->id == X86_INS_SAHF) {
        generate_sahf_alternative(b, insn);
    } else {
        buffer_append(b, insn->bytes, insn->size);
    }
}

// Define the strategy structure
strategy_t lahf_sahf_flag_preservation_strategy = {
    .name = "LAHF/SAHF Flag Preservation",
    .can_handle = can_handle_lahf_sahf_alternative,
    .get_size = get_size_lahf_sahf_alternative,
    .generate = generate_lahf_sahf_alternative,
    .priority = 83  // As specified in documentation
};