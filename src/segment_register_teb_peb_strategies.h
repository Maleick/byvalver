#ifndef SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H
#define SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * Segment Register TEB/PEB Access Strategies
 *
 * PURPOSE: Exploit FS/GS segment registers to access Thread Environment Block (TEB)
 * and Process Environment Block (PEB) without using immediate values that contain
 * bad characters, particularly useful for Windows shellcode.
 */

typedef struct {
    // Base strategy info
    strategy_t base;

    // Strategy-specific parameters
    int segment_register;  // X86_REG_FS or X86_REG_GS
    int target_offset;     // Offset within TEB/PEB structure
    int access_method;     // 0=direct, 1=indirect via register
} segment_register_strategy_t;

// Strategy interface functions
int can_handle_segment_register_strategy(cs_insn *insn);
size_t get_size_segment_register_strategy(cs_insn *insn);
void apply_segment_register_strategy(struct buffer *b, cs_insn *insn);
int transform_fs_gs_access(cs_insn *insn, struct buffer *b);
int transform_teb_peb_access(cs_insn *insn, struct buffer *b);

#endif /* SEGMENT_REGISTER_TEB_PEB_STRATEGIES_H */