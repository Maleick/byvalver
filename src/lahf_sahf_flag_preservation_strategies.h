#ifndef LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H
#define LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/*
 * LAHF/SAHF Flag Preservation Chain Strategies
 *
 * PURPOSE: Use LAHF (Load AH from Flags) and SAHF (Store AH into Flags) 
 * instructions as lightweight alternatives to PUSHF/POPF for preserving
 * arithmetic flags (SF, ZF, AF, PF, CF) without modifying the stack.
 *
 * PRIORITY: 83 (High)
 *
 * EXAMPLES:
 *   Original: pushf; ...; popf (3+ bytes, modifies stack)
 *   Optimized: lahf; ...; sahf (2 bytes total, no stack impact)
 */

// Strategy interface functions
int can_handle_lahf_sahf_flag_preservation(cs_insn *insn);
size_t get_size_lahf_sahf_flag_preservation(cs_insn *insn);
void generate_lahf_sahf_flag_preservation(struct buffer *b, cs_insn *insn);

// Registration function
void register_lahf_sahf_flag_preservation_strategies(void);

#endif /* LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H */