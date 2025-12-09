#ifndef PEB_API_RESOLUTION_STRATEGIES_H
#define PEB_API_RESOLUTION_STRATEGIES_H

#include "strategy.h"

// Enhanced PEB-based API Resolution Strategies
// Handles sophisticated Windows API resolution patterns that involve 
// PEB traversal and hash-based API lookups which commonly contain null bytes

// Function declarations for peb_api_resolution_strategies.c
int can_handle_enhanced_peb_traversal(cs_insn *insn);
int can_handle_hash_based_resolution(cs_insn *insn);
int can_handle_peb_conditional_jumps(cs_insn *insn);
size_t get_size_enhanced_peb_traversal(cs_insn *insn);
size_t get_size_hash_based_resolution(cs_insn *insn);
size_t get_size_peb_conditional_jumps(cs_insn *insn);
void generate_enhanced_peb_traversal(struct buffer *b, cs_insn *insn);
void generate_hash_based_resolution(struct buffer *b, cs_insn *insn);
void generate_peb_conditional_jumps(struct buffer *b, cs_insn *insn);

// Strategy definitions
extern strategy_t enhanced_peb_traversal_strategy;
extern strategy_t hash_based_resolution_strategy;
extern strategy_t peb_conditional_jump_strategy;

// Registration function
void register_peb_api_resolution_strategies();

#endif