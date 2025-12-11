/*
 * BYVALVER - Runtime Self-Modification Obfuscation (Priority 99)
 *
 * Implements runtime self-modifying code where instructions are encoded with
 * marker bytes and decoded at runtime before execution. This provides:
 * - Signature evasion (static signatures cannot match encoded instructions)
 * - Emulator resistance (sandbox emulators may not simulate self-modification)
 * - IDS/IPS bypass (network-based detection cannot identify encoded patterns)
 * - Code morphing (each execution can use different encoding markers)
 *
 * Example transformation:
 *   INT 0x80 (0xCD80) → marker 0x7DCA
 *   Decoder loop transforms: 0x7DCA + 0x303 = 0x80CD at runtime
 */

#ifndef RUNTIME_SELFMOD_OBFUSCATION_H
#define RUNTIME_SELFMOD_OBFUSCATION_H

void register_runtime_selfmod_obfuscation();

#endif // RUNTIME_SELFMOD_OBFUSCATION_H
