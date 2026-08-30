#ifndef BLAKE2S_H
#define BLAKE2S_H
#include <stdint.h>
#include <stddef.h>

#define BLAKE2S_OUTBYTES  32
#define BLAKE2S_BLOCKBYTES 64
#define BLAKE2S_KEYBYTES  32

/**
 * @brief BLAKE2s state context
 */
typedef struct {
    uint32_t h[8];        /* chaining values */
    uint32_t t[2];        /* counter (128-bit) */
    uint32_t f[2];        /* finalization flags */
    uint8_t  buf[BLAKE2S_BLOCKBYTES];
    size_t   buflen;
    uint8_t  outlen;
} blake2s_state;

/**
 * @brief One-shot hash: hash 'inlen' bytes from 'in', write 32 bytes to 'out'
 * @param out Output buffer of 32 bytes
 * @param in Input buffer
 * @param inlen Input length in bytes
 */
void blake2s(uint8_t out[BLAKE2S_OUTBYTES], const void *in, size_t inlen);

/**
 * @brief One-shot hash (alias for blake2s()). Phase 2 API name.
 * @param out    Output buffer of BLAKE2S_OUTBYTES (32) bytes
 * @param in     Input data
 * @param inlen  Input length in bytes
 */
void blake2s_hash(uint8_t out[BLAKE2S_OUTBYTES], const void *in, size_t inlen);

/**
 * @brief Streaming interface - initialization
 * @param S State context
 */
void blake2s_init(blake2s_state *S);

/**
 * @brief Streaming interface - update
 * @param S State context
 * @param in Input buffer
 * @param inlen Input length in bytes
 */
void blake2s_update(blake2s_state *S, const void *in, size_t inlen);

/**
 * @brief Streaming interface - finalization
 * @param S State context
 * @param out Output buffer of 32 bytes
 */
void blake2s_final(blake2s_state *S, uint8_t out[BLAKE2S_OUTBYTES]);

#endif /* BLAKE2S_H */
