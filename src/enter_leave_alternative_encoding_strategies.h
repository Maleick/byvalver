#ifndef ENTER_LEAVE_ALTERNATIVE_ENCODING_STRATEGIES_H
#define ENTER_LEAVE_ALTERNATIVE_ENCODING_STRATEGIES_H

#include "strategy.h"
#include "utils.h"

/*
 * ENTER/LEAVE Stack Frame Alternative Encoding Strategy
 *
 * PURPOSE: Replace ENTER/LEAVE instructions that contain bad characters with
 * equivalent manual stack frame operations.
 *
 * TECHNIQUE:
 * ENTER and LEAVE provide compact function prologue/epilogue but may encode
 * with null bytes in the immediate values.
 *
 * - ENTER imm16, imm8 (C8 iw ib): Create stack frame with imm16 bytes of local space
 * - LEAVE (C9): Destroy stack frame (MOV ESP, EBP; POP EBP)
 *
 * Transformations:
 * 1. ENTER imm16, 0 => PUSH EBP; MOV EBP, ESP; SUB ESP, imm16
 * 2. LEAVE => MOV ESP, EBP; POP EBP
 *
 * Example:
 * enter 0x0100, 0  (C8 00 01 00) - has null bytes
 * =>
 * push ebp         (55)
 * mov ebp, esp     (89 E5)
 * sub esp, 0x100   (81 EC 00 01 00 00) - may need further transformation for null-free
 *
 * NOTE:
 * - ENTER is rare in shellcode (<5% usage)
 * - Manual prologue is actually faster on modern CPUs
 * - LEAVE is more common but straightforward to replace
 *
 * PRIORITY: 74 (Very Low - rare in shellcode, but easy to implement)
 */

// Strategy interface functions
int can_handle_enter_leave(cs_insn *insn);
size_t get_size_enter_leave(cs_insn *insn);
void generate_enter_leave(struct buffer *b, cs_insn *insn);

#endif /* ENTER_LEAVE_ALTERNATIVE_ENCODING_STRATEGIES_H */
