/*
 * BYVALVER - Syscall Instruction Substitution (Priority 79)
 *
 * Replaces standard syscall mechanisms with alternative invocation methods.
 * This provides:
 * - Syscall hooking evasion (some hooks only monitor INT 0x80)
 * - IDS signature evasion (breaks common syscall patterns)
 * - Modern kernel support (leverages VDSO/vsyscall optimizations)
 * - Behavioral variation (different syscall methods complicate analysis)
 *
 * Example transformations:
 *   INT 0x80 → CALL [__kernel_vsyscall] (via sysenter)
 *   INT 0x80 → indirect via function pointer
 */

#ifndef SYSCALL_INSTRUCTION_SUBSTITUTION_H
#define SYSCALL_INSTRUCTION_SUBSTITUTION_H

void register_syscall_instruction_substitution();

#endif // SYSCALL_INSTRUCTION_SUBSTITUTION_H
