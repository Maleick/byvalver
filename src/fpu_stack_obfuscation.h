/*
 * BYVALVER - FPU Stack Obfuscation (Priority 86)
 *
 * Leverages FPU (x87) instructions for obfuscation purposes. This provides:
 * - Rare instruction use (analyzers often ignore FPU instructions)
 * - Position-independent code (FSTENV provides EIP without CALL/POP)
 * - Complex data flow (FPU stack operations obscure value tracking)
 * - Anti-emulation (some emulators poorly handle FPU state)
 *
 * Example transformations:
 *   MOV EAX, imm → FPU arithmetic to compute value
 *   GetPC technique → FSTENV [ESP-12]; POP EAX; POP EAX
 */

#ifndef FPU_STACK_OBFUSCATION_H
#define FPU_STACK_OBFUSCATION_H

void register_fpu_stack_obfuscation();

#endif // FPU_STACK_OBFUSCATION_H
