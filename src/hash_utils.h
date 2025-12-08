#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <stdint.h>

uint32_t ror13_hash(const char *name);
uint32_t rol5_add_hash(const char *name);

#endif // HASH_UTILS_H
