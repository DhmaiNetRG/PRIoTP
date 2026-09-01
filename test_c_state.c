#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define ASCON_AEAD128_IV  0x80400c0600000000ULL

struct ascon_state {
    uint64_t x0, x1, x2, x3, x4;
};

static uint64_t be64_load(const uint8_t *b) {
    return  ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48)
          | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32)
          | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16)
          | ((uint64_t)b[6] <<  8) | ((uint64_t)b[7]);
}

static const uint64_t ROUND_CONSTANTS[12] = {
    0x000000000000003cULL, 0x000000000000002dULL, 0x000000000000001eULL,
    0x000000000000000fULL, 0x00000000000000f0ULL, 0x00000000000000e1ULL,
    0x00000000000000d2ULL, 0x00000000000000c3ULL, 0x00000000000000b4ULL,
    0x00000000000000a5ULL, 0x0000000000000096ULL, 0x0000000000000087ULL
};

static void ascon_round(struct ascon_state *s, uint64_t rc) {
    uint64_t t0, t1, t2, t3, t4;
    s->x2 ^= rc;
    s->x0 ^= s->x4;   s->x4 ^= s->x3;   s->x2 ^= s->x1;
    t0 = s->x0; t1 = s->x1; t2 = s->x2; t3 = s->x3; t4 = s->x4;
    s->x0 = t0 ^ (~t1 & t2); s->x1 = t1 ^ (~t2 & t3); s->x2 = t2 ^ (~t3 & t4); s->x3 = t3 ^ (~t4 & t0); s->x4 = t4 ^ (~t0 & t1);
    s->x1 ^= s->x0;   s->x0 ^= s->x4;   s->x3 ^= s->x2;   s->x2 = ~s->x2;

    s->x0 ^= ((s->x0 >> 19) | (s->x0 << 45)) ^ ((s->x0 >> 28) | (s->x0 << 36));
    s->x1 ^= ((s->x1 >> 61) | (s->x1 <<  3)) ^ ((s->x1 >> 39) | (s->x1 << 25));
    s->x2 ^= ((s->x2 >>  1) | (s->x2 << 63)) ^ ((s->x2 >>  6) | (s->x2 << 58));
    s->x3 ^= ((s->x3 >> 10) | (s->x3 << 54)) ^ ((s->x3 >> 17) | (s->x3 << 47));
    s->x4 ^= ((s->x4 >>  7) | (s->x4 << 57)) ^ ((s->x4 >> 41) | (s->x4 << 23));
}

int main() {
    uint8_t key[16], nonce[16];
    const char *key_hex = "bf473b6dd5d29bc4d9b4e2d4483c0716";
    const char *nonce_hex = "00040000000000010000000000000000";
    for(int i=0; i<16; i++) { sscanf(key_hex + 2*i, "%2hhx", &key[i]); sscanf(nonce_hex + 2*i, "%2hhx", &nonce[i]); }
    
    struct ascon_state s;
    s.x0 = ASCON_AEAD128_IV;
    s.x1 = be64_load(key);
    s.x2 = be64_load(key + 8);
    s.x3 = be64_load(nonce);
    s.x4 = be64_load(nonce + 8);

    ascon_round(&s, ROUND_CONSTANTS[0]); // Round 0
    
    printf("State after round 1:\n");
    printf("x0=%016llx x1=%016llx x2=%016llx x3=%016llx x4=%016llx\n", 
            (unsigned long long)s.x0, (unsigned long long)s.x1, 
            (unsigned long long)s.x2, (unsigned long long)s.x3, 
            (unsigned long long)s.x4);
    
    return 0;
}
