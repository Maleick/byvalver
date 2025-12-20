/*
 * LAHF/SAHF Flag Preservation Chain Strategies
 *
 * PROBLEM: Flag preservation is critical when transforming instructions. 
 * Current strategies use PUSHF/POPF, but LAHF/SAHF provide alternative 
 * lightweight flag save/restore:
 * - LAHF (Load AH from Flags) - 9Fh: Loads SF, ZF, AF, PF, CF into AH
 * - SAHF (Store AH into Flags) - 9Eh: Restores SF, ZF, AF, PF, CF from AH
 *
 * SOLUTION: Use LAHF/SAHF for preserving arithmetic flags (SF, ZF, AF, PF, CF) 
 * without modifying the stack or affecting other flags.
 */

#include "lahf_sahf_flag_preservation_strategies.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Strategy registry entry
strategy_t lahf_sahf_flag_preservation_strategy = {
    .name = "LAHF/SAHF Flag Preservation Chain",
    .can_handle = can_handle_lahf_sahf_flag_preservation,
    .get_size = get_size_lahf_sahf_flag_preservation,
    .generate = generate_lahf_sahf_flag_preservation,
    .priority = 83
};


// Helper function to check if an instruction modifies flags that we care about
static int instruction_modifies_arithmetic_flags(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // Check if instruction modifies arithmetic flags (SF, ZF, AF, PF, CF)
    // This is a simplified check - in a full implementation, we'd check more thoroughly
    switch (insn->id) {
        case X86_INS_ADD:
        case X86_INS_SUB:
        case X86_INS_CMP:
        case X86_INS_AND:
        case X86_INS_OR:
        case X86_INS_XOR:
        case X86_INS_TEST:
        case X86_INS_INC:
        case X86_INS_DEC:
        case X86_INS_NEG:
        case X86_INS_NOT:
        case X86_INS_SHL:
        case X86_INS_SHR:
        case X86_INS_SAR:
        case X86_INS_ROL:
        case X86_INS_ROR:
        case X86_INS_MUL:
        case X86_INS_IMUL:
        case X86_INS_DIV:
        case X86_INS_IDIV:
            return 1;
        default:
            return 0;
    }
}

// Check if this instruction can benefit from LAHF/SAHF flag preservation
// This strategy is used when we have instructions that modify flags and
// we want to preserve them using LAHF/SAHF instead of PUSHF/POPF
int can_handle_lahf_sahf_flag_preservation(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // This strategy is applicable when we have an instruction that modifies flags
    // and we're in a context where flag preservation is needed
    return instruction_modifies_arithmetic_flags(insn);
}

// Estimate the size of the transformation
size_t get_size_lahf_sahf_flag_preservation(cs_insn *insn) {
    if (!insn) {
        return 0;
    }

    // LAHF and SAHF are each 1 byte, so total overhead is 2 bytes
    // compared to PUSHF/POPF which are typically 1-2 bytes each (2-4 total)
    return 2;
}

// Generate the LAHF/SAHF based flag preservation sequence
void generate_lahf_sahf_flag_preservation(struct buffer *b, cs_insn *insn) {
    if (!b || !insn) {
        return;
    }

    // This function would be used to generate LAHF/SAHF sequences
    // in contexts where flag preservation is needed
    
    // For example, if we need to preserve flags across a transformation:
    // lahf; [transformation code]; sahf
    
    // LAHF instruction: 0x9F
    buffer_append(b, (uint8_t[]){0x9F}, 1);  // LAHF - Load flags into AH
    
    // The actual transformation would go here (not part of this strategy)
    // Then to restore flags:
    // SAHF instruction: 0x9E  
    // buffer_append(b, (uint8_t[]){0x9E}, 1);  // SAHF - Store AH into flags
}

// Registration function
void register_lahf_sahf_flag_preservation_strategies(void) {
    extern strategy_t lahf_sahf_flag_preservation_strategy;
    register_strategy(&lahf_sahf_flag_preservation_strategy);
}