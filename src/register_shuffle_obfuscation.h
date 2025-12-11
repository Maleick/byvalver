/*
 * BYVALVER - Register Shuffling Obfuscation (Priority 82)
 *
 * Inserts register exchange operations to confuse data flow analysis.
 * This provides:
 * - Data flow obfuscation (makes dependency analysis difficult)
 * - Pattern breaking (standard calling conventions are obscured)
 * - Polymorphic variants (different register patterns = different signatures)
 * - Semantic preservation (functionality remains identical)
 *
 * Example transformations:
 *   instruction → XCHG EAX, EBX; XCHG EAX, EBX; instruction
 *   instruction → PUSH EAX; XCHG [ESP], EBX; POP EAX; instruction
 */

#ifndef REGISTER_SHUFFLE_OBFUSCATION_H
#define REGISTER_SHUFFLE_OBFUSCATION_H

void register_register_shuffle_obfuscation();

#endif // REGISTER_SHUFFLE_OBFUSCATION_H
