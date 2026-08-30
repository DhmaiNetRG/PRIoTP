/**
 * @file ascon.c
 * @brief ASCON-AEAD128 lightweight authenticated encryption implementation.
 *
 * Implements ASCON-AEAD128 as defined in the ASCON v1.2 specification
 * (Dobraunig, Eichlseder, Mendel, Schläffer, 2021).
 *
 * ASCON-AEAD128 algorithm summary:
 *   - State: 5 × 64-bit words = 320 bits
 *   - Rate:  64 bits (x0 word)
 *   - Key:   128 bits (k0 || k1)
 *   - Nonce: 128 bits (n0 || n1)
 *   - Tag:   128 bits (t0 || t1)
 *   - pa:    12 rounds (initialization, finalization)
 *   - pb:    6 rounds  (associated data, encryption/decryption)
 *
 * Initialization vector (IV) for ASCON-AEAD128:
 *   IV = 0x80400C0600000000  (per spec, section 2.2)
 *
 * This implementation is portable C99 with no heap allocation,
 * no library dependencies beyond stdint.h, string.h.
 *
 * @note Big-endian byte order is used at all rate word boundaries,
 *       as mandated by the ASCON specification.
 */

#include "ascon.h"

#include <string.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Byte-order helpers (portable, no compiler intrinsics)
 * -------------------------------------------------------------------------- */

static uint64_t be64_load(const uint8_t *b)
{
    return  ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48)
          | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32)
          | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16)
          | ((uint64_t)b[6] <<  8) | ((uint64_t)b[7]);
}

static void be64_store(uint8_t *b, uint64_t v)
{
    b[0] = (uint8_t)(v >> 56);
    b[1] = (uint8_t)(v >> 48);
    b[2] = (uint8_t)(v >> 40);
    b[3] = (uint8_t)(v >> 32);
    b[4] = (uint8_t)(v >> 24);
    b[5] = (uint8_t)(v >> 16);
    b[6] = (uint8_t)(v >>  8);
    b[7] = (uint8_t)(v);
}

/* Partial-word load: reads exactly 'n' bytes (1..7) big-endian into uint64_t */
static uint64_t be64_load_partial(const uint8_t *b, size_t n)
{
    uint64_t v = 0;
    size_t i;
    for (i = 0; i < n; i++)
        v |= ((uint64_t)b[i] << (56 - 8*i));
    return v;
}

/* Partial-word store: writes exactly 'n' high bytes of v into b */
static void be64_store_partial(uint8_t *b, uint64_t v, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        b[i] = (uint8_t)(v >> (56 - 8*i));
}

/* --------------------------------------------------------------------------
 * ASCON permutation
 * -------------------------------------------------------------------------- */

/*
 * Round constant schedule (12 rounds, for pa).
 * For pb (6 rounds), only the last 6 are used: start_round = 6.
 * rc[i] = (0xf0 - i*0x10) | (0xc - i)  from the ASCON spec.
 */
static const uint64_t ROUND_CONSTANTS[12] = {
    0x000000000000003cULL,  /* round  0 */
    0x000000000000002dULL,  /* round  1 */
    0x000000000000001eULL,  /* round  2 */
    0x000000000000000fULL,  /* round  3 */
    0x00000000000000f0ULL,  /* round  4 */
    0x00000000000000e1ULL,  /* round  5 */
    0x00000000000000d2ULL,  /* round  6 */
    0x00000000000000c3ULL,  /* round  7 */
    0x00000000000000b4ULL,  /* round  8 */
    0x00000000000000a5ULL,  /* round  9 */
    0x0000000000000096ULL,  /* round 10 */
    0x0000000000000087ULL   /* round 11 */
};

/**
 * @brief Execute one ASCON round on the state.
 *
 * Applies:  p_C (constant addition), p_S (substitution), p_L (linear diffusion)
 */
static void ascon_round(struct ascon_state *s, uint64_t rc)
{
    uint64_t t0, t1, t2, t3, t4;

    /* --- p_C: constant addition ----------------------------------------- */
    s->x2 ^= rc;

    /* --- p_S: 5-bit S-box layer ----------------------------------------- */
    s->x0 ^= s->x4;   s->x4 ^= s->x3;   s->x2 ^= s->x1;
    t0 = s->x0;        t1 = s->x1;        t2 = s->x2;        t3 = s->x3;        t4 = s->x4;
    s->x0 = t0 ^ (~t1 & t2);
    s->x1 = t1 ^ (~t2 & t3);
    s->x2 = t2 ^ (~t3 & t4);
    s->x3 = t3 ^ (~t4 & t0);
    s->x4 = t4 ^ (~t0 & t1);
    s->x1 ^= s->x0;   s->x0 ^= s->x4;   s->x3 ^= s->x2;   s->x2 = ~s->x2;

    /* --- p_L: linear diffusion layer ------------------------------------ */
    s->x0 ^= ((s->x0 >> 19) | (s->x0 << 45)) ^
             ((s->x0 >> 28) | (s->x0 << 36));
    s->x1 ^= ((s->x1 >> 61) | (s->x1 <<  3)) ^
             ((s->x1 >> 39) | (s->x1 << 25));
    s->x2 ^= ((s->x2 >>  1) | (s->x2 << 63)) ^
             ((s->x2 >>  6) | (s->x2 << 58));
    s->x3 ^= ((s->x3 >> 10) | (s->x3 << 54)) ^
             ((s->x3 >> 17) | (s->x3 << 47));
    s->x4 ^= ((s->x4 >>  7) | (s->x4 << 57)) ^
             ((s->x4 >> 41) | (s->x4 << 23));
}

/**
 * @brief Apply pa (12-round) or pb (6-round) permutation.
 * @param s          Permutation state.
 * @param start_round  0 for pa (12 rounds), 6 for pb (6 rounds).
 */
static void ascon_permute(struct ascon_state *s, int start_round)
{
    int i;
    for (i = start_round; i < 12; i++)
        ascon_round(s, ROUND_CONSTANTS[i]);
}

/* Shorthand macros */
#define ascon_pa(s)  ascon_permute((s), 0)
#define ascon_pb(s)  ascon_permute((s), 6)

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 initialization
 *
 * IV for ASCON-AEAD128 = 0x80400C0600000000 (per spec section 2.2)
 *   bits: key_len=128, rate=64, pa=12, pb=6, zero-padding=32
 * -------------------------------------------------------------------------- */
#define ASCON_AEAD128_IV  0x80400c0600000000ULL

static void ascon_init(struct ascon_state *s,
                       const uint8_t *key,
                       const uint8_t *nonce)
{
    uint64_t k0 = be64_load(key);
    uint64_t k1 = be64_load(key + 8);
    uint64_t n0 = be64_load(nonce);
    uint64_t n1 = be64_load(nonce + 8);

    s->x0 = ASCON_AEAD128_IV;
    s->x1 = k0;
    s->x2 = k1;
    s->x3 = n0;
    s->x4 = n1;

    ascon_pa(s);

    s->x3 ^= k0;
    s->x4 ^= k1;
}

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 associated data processing
 * -------------------------------------------------------------------------- */
static void ascon_process_aad(struct ascon_state *s,
                               const uint8_t *aad,
                               size_t aad_len)
{
    /* Absorb full 8-byte (64-bit) blocks */
    while (aad_len >= ASCON_RATE) {
        s->x0 ^= be64_load(aad);
        ascon_pb(s);
        aad     += ASCON_RATE;
        aad_len -= ASCON_RATE;
    }

    /* Absorb final partial block with 0x80 domain-separation padding */
    {
        uint64_t last = 0;
        if (aad_len > 0)
            last = be64_load_partial(aad, aad_len);
        last ^= (uint64_t)0x80 << (56 - 8 * aad_len);
        s->x0 ^= last;
        ascon_pb(s);
    }

    /* Domain separation bit between AAD and plaintext */
    s->x4 ^= 0x0000000000000001ULL;
}

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 finalization (tag generation / verification)
 * -------------------------------------------------------------------------- */
static void ascon_finalize(struct ascon_state *s,
                            const uint8_t *key,
                            uint8_t *tag)
{
    uint64_t k0 = be64_load(key);
    uint64_t k1 = be64_load(key + 8);

    /* Key XOR into rate capacity boundary */
    s->x1 ^= k0;
    s->x2 ^= k1;

    ascon_pa(s);

    /* Extract tag: last two words XOR'd with key */
    s->x3 ^= k0;
    s->x4 ^= k1;

    be64_store(tag,     s->x3);
    be64_store(tag + 8, s->x4);
}

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 encrypt
 * -------------------------------------------------------------------------- */

void ascon_aead128_encrypt(
    uint8_t       *ciphertext_and_tag,
    const uint8_t *plaintext,
    size_t         plaintext_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key)
{
    struct ascon_state s;
    uint8_t       *ct  = ciphertext_and_tag;
    const uint8_t *pt  = plaintext;
    size_t         len = plaintext_len;
    uint64_t       block;

    /* 1. Initialization */
    ascon_init(&s, key, nonce);

    /* 2. Associated data (if any) */
    if (aad_len > 0)
        ascon_process_aad(&s, aad, aad_len);
    else
        s.x4 ^= 0x0000000000000001ULL;  /* domain sep even with empty AAD */

    /* 3. Encrypt plaintext blocks */
    while (len >= ASCON_RATE) {
        block   = be64_load(pt);
        s.x0   ^= block;
        be64_store(ct, s.x0);
        ascon_pb(&s);
        pt  += ASCON_RATE;
        ct  += ASCON_RATE;
        len -= ASCON_RATE;
    }

    /* Final partial block with padding */
    {
        uint64_t last = 0;
        if (len > 0)
            last = be64_load_partial(pt, len);
        last ^= (uint64_t)0x80 << (56 - 8 * len);
        s.x0 ^= last;
        be64_store_partial(ct, s.x0, len);
        ct += len;
    }

    /* 4. Finalization — append 16-byte tag */
    ascon_finalize(&s, key, ct);
}

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 decrypt
 * -------------------------------------------------------------------------- */

int ascon_aead128_decrypt(
    uint8_t       *plaintext,
    const uint8_t *ciphertext_with_tag,
    size_t         ciphertext_with_tag_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key)
{
    struct ascon_state s;
    const uint8_t *ct;
    uint8_t       *pt;
    size_t         ct_len;
    uint64_t       block;
    uint8_t        computed_tag[ASCON_TAG_LEN];
    uint8_t        diff;
    int            i;

    /* Reject if too short to contain even an empty message + tag */
    if (ciphertext_with_tag_len < ASCON_TAG_LEN)
        return -1;

    ct_len = ciphertext_with_tag_len - ASCON_TAG_LEN;
    ct     = ciphertext_with_tag;
    pt     = plaintext;

    /* 1. Initialization */
    ascon_init(&s, key, nonce);

    /* 2. Associated data (if any) */
    if (aad_len > 0)
        ascon_process_aad(&s, aad, aad_len);
    else
        s.x4 ^= 0x0000000000000001ULL;  /* domain sep even with empty AAD */

    /* 3. Decrypt ciphertext blocks */
    {
        size_t len = ct_len;
        while (len >= ASCON_RATE) {
            block  = be64_load(ct);
            be64_store(pt, s.x0 ^ block);
            s.x0   = block;
            ascon_pb(&s);
            ct  += ASCON_RATE;
            pt  += ASCON_RATE;
            len -= ASCON_RATE;
        }

        /* Final partial block */
        {
            uint64_t last_ct = 0;
            uint64_t last_pt;
            if (len > 0)
                last_ct = be64_load_partial(ct, len);
            last_pt = s.x0 ^ last_ct;
            if (len > 0)
                be64_store_partial(pt, last_pt, len);

            /* Restore ciphertext into state (overwrite only the absorbed bytes) */
            {
                uint64_t mask = 0;
                if (len > 0)
                    mask = ~0ULL << (64 - 8 * len);
                
                s.x0 = (s.x0 & ~mask) | (last_ct & mask);
            }
            /* Padding bit */
            s.x0 ^= (uint64_t)0x80 << (56 - 8 * len);
        }
    }

    /* 4. Compute expected tag */
    ascon_finalize(&s, key, computed_tag);

    /* 5. Constant-time tag comparison (prevent timing side-channels) */
    diff = 0;
    for (i = 0; i < ASCON_TAG_LEN; i++)
        diff |= computed_tag[i] ^ ciphertext_with_tag[ct_len + i];

    /* Zero the plaintext buffer on auth failure to prevent leakage */
    if (diff != 0) {
        memset(plaintext, 0, ct_len);
        return -1;
    }

    return 0;
}

/* --------------------------------------------------------------------------
 * Phase 2 API: short-name aliases
 * -------------------------------------------------------------------------- */

/**
 * @brief Encrypt with ASCON-AEAD128 (Phase 2 API alias).
 */
void ascon_encrypt(
    uint8_t       *ciphertext_and_tag,
    const uint8_t *plaintext,
    size_t         plaintext_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key)
{
    ascon_aead128_encrypt(ciphertext_and_tag, plaintext, plaintext_len,
                           aad, aad_len, nonce, key);
}

/**
 * @brief Decrypt with ASCON-AEAD128 (Phase 2 API alias).
 */
int ascon_decrypt(
    uint8_t       *plaintext,
    const uint8_t *ciphertext_with_tag,
    size_t         ciphertext_with_tag_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key)
{
    return ascon_aead128_decrypt(plaintext, ciphertext_with_tag,
                                  ciphertext_with_tag_len,
                                  aad, aad_len, nonce, key);
}
