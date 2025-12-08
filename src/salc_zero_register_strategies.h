#ifndef SALC_ZERO_REGISTER_STRATEGIES_H
#define SALC_ZERO_REGISTER_STRATEGIES_H

#include "strategy.h"

// SALC-based register zeroing strategy
extern strategy_t salc_zero_register_strategy;
extern strategy_t salc_zero_al_direct_strategy;

// Function to register the SALC-based zero register strategies
void register_salc_zero_register_strategies();

#endif