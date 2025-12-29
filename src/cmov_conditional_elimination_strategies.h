#ifndef CMOV_CONDITIONAL_ELIMINATION_STRATEGIES_H
#define CMOV_CONDITIONAL_ELIMINATION_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * CMOV Conditional Move Elimination Strategies
 *
 * PURPOSE: Eliminate CMOV instructions that may contain bad bytes in
 * ModR/M encoding bytes or displacement, replacing them with equivalent
 * conditional logic using SETcc and arithmetic operations.
 */

typedef struct {
    // Base strategy info
    strategy_t base;

    // Strategy-specific parameters
    int condition_type;     // Type of condition (EQUAL, NOT_EQUAL, etc.)
    int register_used;      // Register affected by CMOV
} cmov_elimination_strategy_t;

// Strategy interface functions
int can_handle_cmov_elimination_strategy(cs_insn *insn);
size_t get_size_cmov_elimination_strategy(cs_insn *insn);
void apply_cmov_elimination_strategy(struct buffer *b, cs_insn *insn);
int transform_cmov_to_setcc_logic(cs_insn *insn, struct buffer *b);
int transform_cmov_with_xor_logic(cs_insn *insn, struct buffer *b);

#endif /* CMOV_CONDITIONAL_ELIMINATION_STRATEGIES_H */