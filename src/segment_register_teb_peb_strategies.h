#ifndef SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H
#define SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * Segment Register TEB/PEB Access Strategy for Bad Character Elimination
 *
 * PURPOSE: Replace FS/GS segment register access patterns that may contain
 * bad bytes in displacement bytes with alternative memory access methods.
 */

// Strategy interface functions
int can_handle_segment_register_access(cs_insn *insn);
size_t get_size_segment_register_access(cs_insn *insn);
void generate_segment_register_access(struct buffer *b, cs_insn *insn);
int can_handle_segment_alternative_access(cs_insn *insn);
size_t get_size_segment_alternative_access(cs_insn *insn);
void generate_segment_alternative_access(struct buffer *b, cs_insn *insn);

// Registration function
void register_segment_register_teb_peb_strategies();

#endif /* SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H */