#ifndef LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H
#define LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * LAHF/SAHF Flag Preservation Strategy for Bad Character Elimination
 *
 * PURPOSE: Replace LAHF/SAHF instructions that may contain bad characters
 * with equivalent flag manipulation using PUSHF/POPF or manual flag handling.
 */

// Strategy interface functions
int can_handle_lahf_alternative(cs_insn *insn);
size_t get_size_lahf_alternative(cs_insn *insn);
void generate_lahf_alternative(struct buffer *b, cs_insn *insn);
int can_handle_sahf_alternative(cs_insn *insn);
size_t get_size_sahf_alternative(cs_insn *insn);
void generate_sahf_alternative(struct buffer *b, cs_insn *insn);
int can_handle_lahf_sahf_alternative(cs_insn *insn);
size_t get_size_lahf_sahf_alternative(cs_insn *insn);
void generate_lahf_sahf_alternative(struct buffer *b, cs_insn *insn);

// Registration function
void register_lahf_sahf_flag_preservation_strategies();

#endif /* LAHF_SAHF_FLAG_PRESERVATION_STRATEGIES_H */