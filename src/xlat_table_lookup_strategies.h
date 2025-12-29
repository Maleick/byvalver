#ifndef XLAT_TABLE_LOOKUP_STRATEGIES_H
#define XLAT_TABLE_LOOKUP_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * XLAT Table Lookup Strategy for Bad Character Elimination
 *
 * PURPOSE: Replace XLAT instructions that may contain bad bytes in
 * table addresses with equivalent logic using MOV from memory with
 * alternative addressing modes.
 */

// Strategy interface functions
int can_handle_xlat_to_mov_substitution(cs_insn *insn);
size_t get_size_xlat_to_mov_substitution(cs_insn *insn);
void generate_xlat_to_mov_substitution(struct buffer *b, cs_insn *insn);
int can_handle_xlat_bad_byte_table(cs_insn *insn);
size_t get_size_xlat_bad_byte_table(cs_insn *insn);
void generate_xlat_bad_byte_table(struct buffer *b, cs_insn *insn);

// Registration function
void register_xlat_table_lookup_strategies();

#endif /* XLAT_TABLE_LOOKUP_STRATEGIES_H */