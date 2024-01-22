#include "ui_hash.h"

#include <cstdio>

#include "constants.h"

bool is_same_hash(hash_t& hash1, hash_t& hash2) {
    for (int i = 0; i < 8; i++) {
        if (hash1.unsigned_ints[i] != hash2.unsigned_ints[i]) {
            return false;
        }
    }
    return true;
}

void print_byte(uint8_t in) {
    for (int i = 7; i >= 0; i--) {
        uint8_t bit = (in >> i) & (0x01);
        if (bit) {
            printf("1");
        } else {
            printf("0");
        }
    }
}

void print_512(uint512_t& in) {
    for (int i = 0; i < 64; i++) {
        uint8_t v = in.unsigned_bytes[i];
        // printf(" %b ", v);
        print_byte(v);
        printf(" ");
    }
    printf("\n");
}

void print_sha(hash_t& sha) {
	printf("%0llx %0llx %0llx %0llx", sha.unsigned_double[0], sha.unsigned_double[1], sha.unsigned_double[2], sha.unsigned_double[3]);
}

uint32_t wrap(uint32_t in, int num) {
    uint32_t res = 0;
    res = in >> num;
    uint32_t top = in << (32 - num);
    res = res | num;
    return res;
}

uint32_t bitwise_add_mod2(uint32_t a, uint32_t b, uint32_t c) {
    // return (~a & ~b & c) | (~a & b & ~c) | (a & b & c) | (a & ~b & ~c);
    return a ^ b ^ c;
}

uint32_t epsilon0(uint32_t in) {
    uint32_t wrap7 = wrap(in, 7);
    uint32_t wrap18 = wrap(in, 18);
    uint32_t shift3 = in >> 3; 
    return bitwise_add_mod2(wrap7, wrap18, shift3);
}

uint32_t epsilon1(uint32_t in) {
    uint32_t wrap17 = wrap(in, 17);
    uint32_t wrap19 = wrap(in, 19);
    uint32_t shift10 = in >> 10; 
    return bitwise_add_mod2(wrap17, wrap19, shift10);
}

uint32_t sigma0(uint32_t in) {
    uint32_t wrap2 = wrap(in, 2);
    uint32_t wrap13 = wrap(in, 13);
    uint32_t wrap22 = wrap(in, 22);
    return bitwise_add_mod2(wrap2, wrap13, wrap22);
}

uint32_t sigma1(uint32_t in) {
    uint32_t wrap6 = wrap(in, 6);
    uint32_t wrap11 = wrap(in, 11);
    uint32_t wrap25 = wrap(in, 25);
    return bitwise_add_mod2(wrap6, wrap11, wrap25);
}

uint32_t maj(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

uint32_t ch(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (~a & c);
}

uint32_t calc_t1(working_variables_t& cur_vars, uint32_t* w, int t) {

    static uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint64_t interm = cur_vars.h + sigma1(cur_vars.e) + ch(cur_vars.e, cur_vars.f, cur_vars.g) + k[t] + w[t];
    uint32_t t1 = mod_2_pow_32(interm);
    return t1;
}

uint32_t calc_t2(working_variables_t& cur_vars) {
    uint64_t s0 = sigma0(cur_vars.a);
    uint64_t interm = s0 + maj(cur_vars.a, cur_vars.b, cur_vars.c);
    return mod_2_pow_32(interm);
}

uint32_t mod_2_pow_32(uint64_t in) {
    uint64_t div = 1;
    div = div << 32;
    uint64_t interm = in % div;
    uint64_t mask = 0x0000ffff;
    uint32_t res = interm & mask;
    return res;
}

// not exactly sha 256 hash but does similar hash
// TODO: for exact sha 256 hashing, need to look more closely at bit layouts
// since things in mem r little endian but working with unions means
// insert values cause diff little endian behavior

// ex) settings the size in the last unsigned double looks fine for the double,
// but when interpreted as unsigned ints, it is in M[14] rather than M[15] which
// is need for sha256 exactly

// all in all, sha256 switches between different representations of the same 512 bits,
// but in acc memory this is weird cause diff representation is formatted diff in mem

// but for rn...this is good enough...so for rn...its like sha-256 but not sha-256
hash_t hash(const char* key) {
    uint512_t input{};
    int key_len = strlen(key);
    uint64_t size = 8 * key_len;
    game_assert_msg(size < 512, "key size must be less than 512 bits");
    
    memcpy(&input, key, key_len);

    /*  PADDING */
    // last 8 bytes reserved for the padding size
    input.unsigned_bytes[key_len] = 0x80;
    input.unsigned_double[7] = size;

    // print_sha()
    // print_512(input);

    uint32_t h[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    uint32_t w[64]{};
    for (int t = 0; t < 16; t++) {
        w[t] = input.unsigned_ints[t];
    }

    for (int t = 16; t < 63; t++) {
        int64_t ep1 = epsilon1(w[t-2]);
        int64_t ep0 = epsilon0(w[t-15]);
        uint64_t intermediate = ep1 + w[t-7] + ep0 + w[t-16];
        w[t] = mod_2_pow_32(intermediate);
    }

    working_variables_t working_variables{};
    memcpy(&working_variables, h, sizeof(working_variables_t));

    for (int t = 0; t < 63; t++) {
        working_variables_t cur_vars = working_variables;

        uint64_t t1 = calc_t1(cur_vars, w, t);
        uint64_t t2 = calc_t2(cur_vars);

        uint64_t a_interm = t1 + t2;
        working_variables.a = mod_2_pow_32(a_interm);

        working_variables.b = cur_vars.a;
        working_variables.c = cur_vars.b;
        working_variables.d = cur_vars.c;

        uint64_t e_interm = cur_vars.d + t1;
        working_variables.e = mod_2_pow_32(e_interm);

        working_variables.f = cur_vars.e;
        working_variables.g = cur_vars.f;
        working_variables.h = cur_vars.g;

        cur_vars = working_variables;
    }

    for (int i = 0; i < 8; i++) {
        uint64_t v1 = working_variables.vals[i];
        uint64_t v2 = h[i];
        uint64_t v = v1 + v2;
        h[i] = mod_2_pow_32(v);
    }

    hash_t sha;
    for (int i = 0; i < 8; i++) {
        sha.unsigned_ints[i] = h[i];
    }

    return sha;
}