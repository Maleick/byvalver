/*
 * BYVALVER - Pass 1: Obfuscation Strategies Header
 */

#ifndef OBFUSCATION_STRATEGIES_H
#define OBFUSCATION_STRATEGIES_H

// Individual strategy registration functions
void register_test_to_and_obfuscation();
void register_mov_push_pop_obfuscation();
void register_arithmetic_negation_obfuscation();
void register_junk_code_insertion();
void register_opaque_predicate_obfuscation();
void register_register_renaming_obfuscation();
void register_instruction_reordering();
void register_constant_unfolding();
void register_nop_insertion();
void register_stack_spill_obfuscation();

// NEW: 10 Additional Obfuscation Strategies
void register_runtime_selfmod_obfuscation();
void register_incremental_decoder_obfuscation();
void register_mutated_junk_insertion_obfuscation();
void register_semantic_equivalence_substitution();
void register_fpu_stack_obfuscation();
void register_overlapping_instruction_obfuscation();
void register_register_shuffle_obfuscation();
void register_syscall_instruction_substitution();
void register_control_flow_dispatcher_obfuscation();
void register_mixed_arithmetic_base_obfuscation();

#endif // OBFUSCATION_STRATEGIES_H
