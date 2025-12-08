#include "hash_utils.h"
#include <string.h>

uint32_t ror13_hash(const char *name) {
    uint32_t hash = 0;
    while (*name) {
        hash = (hash >> 13) | (hash << (32 - 13)); // ROR 13
        hash += *name++;
    }
    return hash;
}

// Hashing algorithm based on the one found in shellcode 13533.asm
// hash = ROL(hash, 5) + char
uint32_t rol5_add_hash(const char *name) {
    uint32_t hash = 0;
    while (*name) {
        hash += *name++;
        hash = (hash << 5) | (hash >> (32 - 5)); // ROL 5
    }
    return hash;
}
