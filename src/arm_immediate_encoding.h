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

// Find two immediate values that sum to target and are each ARM-immediate encodable
// Returns 1 if found, 0 otherwise. Stores parts in *part1_out and *part2_out.
int find_arm_addsub_split_immediate(uint32_t target, uint32_t *part1_out, uint32_t *part2_out);

// Check if signed displacement fits ARM LDR/STR immediate offset form (+/- 4095)
int is_arm_displacement_encodable(int32_t displacement);

// Find a two-step displacement split: displacement = pre_adjust + residual
// Returns 1 if found, 0 otherwise. Stores values in outputs.
int find_arm_displacement_split(int32_t displacement, int32_t *pre_adjust_out, int32_t *residual_out);

// Build a displacement rewrite plan with opcode/magnitude metadata for pre-adjust step.
// pre_opcode_out: ADD=0x4 for positive pre-adjust, SUB=0x2 for negative.
int plan_arm_displacement_rewrite(int32_t displacement, int32_t *pre_adjust_out, int32_t *residual_out,
                                  uint32_t *pre_magnitude_out, uint8_t *pre_opcode_out);

// Encode ARM data-processing immediate instruction (I=1 form)
// opcode: ADD=4, SUB=2, etc. cond: 0xE for AL.
int encode_arm_dp_immediate(uint8_t cond, uint8_t opcode, uint8_t rn, uint8_t rd,
                            uint32_t imm, int set_flags, uint32_t *instruction_out);

// Encode ARM LDR/STR immediate instruction (pre-indexed, no writeback)
// is_load: 1=LDR, 0=STR
int encode_arm_ldr_str_immediate(uint8_t cond, int is_load, uint8_t rn, uint8_t rd,
                                 int32_t displacement, uint32_t *instruction_out);

// Decode/encode ARM B-style branch offsets (24-bit signed word offset)
int decode_arm_branch_offset(uint32_t instruction, int32_t *word_offset_out);
int encode_arm_branch_instruction(uint8_t cond, int32_t word_offset, uint32_t *instruction_out);

// Invert a condition code (EQ<->NE, LT<->GE, etc). Returns 1 on success.
int invert_arm_condition(uint8_t cond, uint8_t *inverted_out);

// Map ARM register to index (0-15)
uint8_t get_arm_reg_index(arm_reg reg);

// Check if ARM instruction has bad bytes in the given profile
int arm_has_bad_bytes(cs_insn *insn, const bad_byte_config_t *profile);

#endif /* ARM_IMMEDIATE_ENCODING_H */
