#ifndef XLAT_TABLE_LOOKUP_STRATEGIES_H
#define XLAT_TABLE_LOOKUP_STRATEGIES_H

#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/*
 * XLAT Table-Based Byte Translation Strategies
 *
 * PURPOSE: Use XLAT (translate byte) instruction for table-based byte translation
 * to remap bad characters to safe characters and vice versa. The XLAT instruction
 * performs: AL = [EBX + AL], using AL as an index into a table pointed to by EBX.
 *
 * PRIORITY: 72 (Medium-Low)
 *
 * EXAMPLES:
 *   Original: mov al, 0x00 (has null byte)
 *   Optimized: Build translation table; mov al, safe_value; xlat
 *
 *   Original: Multiple bytes with bad chars
 *   Optimized: Use single translation table for multiple remappings
 */

// Strategy interface functions
int can_handle_xlat_table_lookup(cs_insn *insn);
size_t get_size_xlat_table_lookup(cs_insn *insn);
void generate_xlat_table_lookup(struct buffer *b, cs_insn *insn);

// Registration function
void register_xlat_table_lookup_strategies(void);

#endif /* XLAT_TABLE_LOOKUP_STRATEGIES_H */