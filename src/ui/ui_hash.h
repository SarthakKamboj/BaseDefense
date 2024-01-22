#pragma once

#include <cstdint>

union hash_t {
    uint64_t unsigned_double[4];
    uint32_t unsigned_ints[8];
};

bool is_same_hash(hash_t& hash1, hash_t& hash2);

uint32_t mod_2_pow_32(uint64_t in);
void print_sha(hash_t& sha);

union working_variables_t {
    struct {
        uint32_t a, b, c, d, e, f, g, h; 
    };
    uint32_t vals[8];
};

union uint512_t {
    uint64_t unsigned_double[8];
    uint32_t unsigned_ints[16];
    uint16_t unsigned_shorts[32];
    uint8_t unsigned_bytes[64];
};

void print_512(uint512_t& i);

hash_t hash(const char* key);

