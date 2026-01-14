/*
 * ARM Immediate Encoding Helpers
 *
 * Provides functions for encoding and validating ARM immediate values.
 * ARM immediates are 8-bit values rotated right by even amounts (0-30 bits).
 */

#ifndef ARM_IMMEDIATE_ENCODING_H
#define ARM_IMMEDIATE_ENCODING_H

#include <stdint.h>
#include <capstone/capstone.h>
#include "cli.h"  // For bad_byte_config_t

// Check if a 32-bit value can be encoded as an ARM immediate
int is_arm_immediate_encodable(uint32_t value);

// Encode a 32-bit value as ARM immediate (returns encoded value or -1 if impossible)
int encode_arm_immediate(uint32_t value);

// Find MVN equivalent for values that can't be encoded as MOV immediate
// Returns 1 if found, 0 otherwise. Stores the MVN immediate value in *mvn_val_out
int find_arm_mvn_immediate(uint32_t target, uint32_t *mvn_val_out);

// Map ARM register to index (0-15)
uint8_t get_arm_reg_index(arm_reg reg);

// Check if ARM instruction has bad bytes in the given profile
int arm_has_bad_bytes(cs_insn *insn, const bad_byte_config_t *profile);

#endif /* ARM_IMMEDIATE_ENCODING_H */