#ifndef PEB_API_HASHING_STRATEGIES_H
#define PEB_API_HASHING_STRATEGIES_H

#include "strategy.h"

// Function declarations for peb_api_hashing_strategies.c
int can_handle_peb_api_hashing(cs_insn *insn);
size_t get_size_peb_api_hashing(cs_insn *insn);
void generate_peb_api_hashing(struct buffer *b, cs_insn *insn);

// Strategy definition
extern strategy_t peb_api_hashing_strategy;

#endif