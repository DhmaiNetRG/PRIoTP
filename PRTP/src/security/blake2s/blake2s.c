#include "blake2s.h"
#include <string.h>

static const uint32_t blake2s_iv[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

static const uint8_t blake2s_sigma[10][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 }
};

static inline uint32_t rotr32(const uint32_t w, const unsigned c) {
    return (w >> c) | (w << (32 - c));
}

static inline uint32_t load32(const void *src) {
    const uint8_t *p = (const uint8_t *)src;
    return ((uint32_t)(p[0]) <<  0) |
           ((uint32_t)(p[1]) <<  8) |
           ((uint32_t)(p[2]) << 16) |
           ((uint32_t)(p[3]) << 24);
}

static inline void store32(void *dst, uint32_t w) {
    uint8_t *p = (uint8_t *)dst;
    p[0] = (uint8_t)(w >>  0);
    p[1] = (uint8_t)(w >>  8);
    p[2] = (uint8_t)(w >> 16);
    p[3] = (uint8_t)(w >> 24);
}

static void blake2s_compress(blake2s_state *S, const uint8_t in[BLAKE2S_BLOCKBYTES]) {
    uint32_t m[16];
    uint32_t v[16];
    int i;

    for (i = 0; i < 16; ++i) {
        m[i] = load32(in + i * sizeof(m[i]));
    }

    for (i = 0; i < 8; ++i) {
        v[i] = S->h[i];
    }
    v[ 8] = blake2s_iv[0];
    v[ 9] = blake2s_iv[1];
    v[10] = blake2s_iv[2];
    v[11] = blake2s_iv[3];
    v[12] = S->t[0] ^ blake2s_iv[4];
    v[13] = S->t[1] ^ blake2s_iv[5];
    v[14] = S->f[0] ^ blake2s_iv[6];
    v[15] = S->f[1] ^ blake2s_iv[7];

#define G(r,i,a,b,c,d) do { \
    a = a + b + m[blake2s_sigma[r][2*i+0]]; \
    d = rotr32(d ^ a, 16); \
    c = c + d; \
    b = rotr32(b ^ c, 12); \
    a = a + b + m[blake2s_sigma[r][2*i+1]]; \
    d = rotr32(d ^ a, 8); \
    c = c + d; \
    b = rotr32(b ^ c, 7); \
  } while(0)

#define ROUND(r) do { \
    G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
    G(r,2,v[ 2],v[ 6],v[10],v[14]); \
    G(r,3,v[ 3],v[ 7],v[11],v[15]); \
    G(r,4,v[ 0],v[ 5],v[10],v[15]); \
    G(r,5,v[ 1],v[ 6],v[11],v[12]); \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
  } while(0)

    ROUND(0); ROUND(1); ROUND(2); ROUND(3); ROUND(4);
    ROUND(5); ROUND(6); ROUND(7); ROUND(8); ROUND(9);

#undef G
#undef ROUND

    for (i = 0; i < 8; ++i) {
        S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
    }
}

/**
 * @brief Initialize BLAKE2s state
 * @param S The BLAKE2s state
 */
void blake2s_init(blake2s_state *S) {
    int i;
    memset(S, 0, sizeof(blake2s_state));
    for (i = 0; i < 8; ++i) {
        S->h[i] = blake2s_iv[i];
    }
    /* Parameter block: fan_out=1, depth=1, leaf_len=0, node_offset=0,
     * node_depth=0, inner_len=0, key_len=0 (keyless), out_len=32.
     * h[0] ^= little-endian 32-bit word: 0x01010020
     *   byte 0: digest_length = 32 (0x20)
     *   byte 1: key_length    =  0
     *   byte 2: fanout        =  1
     *   byte 3: depth         =  1
     */
    S->h[0] ^= 0x01010020;  /* keyless: keylen=0, outlen=32 */
    S->outlen = BLAKE2S_OUTBYTES;
}

/**
 * @brief Update BLAKE2s state with input
 * @param S The BLAKE2s state
 * @param in Input buffer
 * @param inlen Input length
 */
void blake2s_update(blake2s_state *S, const void *in, size_t inlen) {
    const uint8_t *pin = (const uint8_t *)in;
    if (inlen > 0) {
        size_t left = S->buflen;
        size_t fill = BLAKE2S_BLOCKBYTES - left;
        if (inlen > fill) {
            S->buflen = 0;
            memcpy(S->buf + left, pin, fill);
            S->t[0] += BLAKE2S_BLOCKBYTES;
            if (S->t[0] < BLAKE2S_BLOCKBYTES) {
                S->t[1]++;
            }
            blake2s_compress(S, S->buf);
            pin += fill;
            inlen -= fill;
            while (inlen > BLAKE2S_BLOCKBYTES) {
                S->t[0] += BLAKE2S_BLOCKBYTES;
                if (S->t[0] < BLAKE2S_BLOCKBYTES) {
                    S->t[1]++;
                }
                blake2s_compress(S, pin);
                pin += BLAKE2S_BLOCKBYTES;
                inlen -= BLAKE2S_BLOCKBYTES;
            }
        }
        memcpy(S->buf + S->buflen, pin, inlen);
        S->buflen += inlen;
    }
}

/**
 * @brief Finalize BLAKE2s state and output hash
 * @param S The BLAKE2s state
 * @param out Output buffer
 */
void blake2s_final(blake2s_state *S, uint8_t out[BLAKE2S_OUTBYTES]) {
    int i;
    S->t[0] += (uint32_t)S->buflen;
    if (S->t[0] < (uint32_t)S->buflen) {
        S->t[1]++;
    }
    while (S->buflen < BLAKE2S_BLOCKBYTES) {
        S->buf[S->buflen++] = 0;
    }
    S->f[0] = 0xFFFFFFFF;
    S->f[1] = 0;
    blake2s_compress(S, S->buf);
    for (i = 0; i < 8; ++i) {
        store32(out + sizeof(S->h[i]) * i, S->h[i]);
    }
}

/**
 * @brief One-shot BLAKE2s hash
 * @param out Output buffer
 * @param in Input buffer
 * @param inlen Input length
 */
void blake2s(uint8_t out[BLAKE2S_OUTBYTES], const void *in, size_t inlen) {
    blake2s_state S;
    blake2s_init(&S);
    blake2s_update(&S, in, inlen);
    blake2s_final(&S, out);
}

/* --------------------------------------------------------------------------
 * Phase 2 API
 * -------------------------------------------------------------------------- */

/**
 * @brief One-shot BLAKE2s-256 hash (Phase 2 alias for blake2s()).
 */
void blake2s_hash(uint8_t out[BLAKE2S_OUTBYTES], const void *in, size_t inlen)
{
    blake2s(out, in, inlen);
}
