/**
 * @file ecies.c
 * @brief ECIES module implementation
 */
#include "ecies.h"
#include "../blake2s/blake2s.h"
#include "../x25519/x25519.h"
#include "../../ascon.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int read_random(uint8_t *buf, size_t n) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t r = fread(buf, 1, n, f);
    fclose(f);
    return (r == n) ? 0 : -1;
}

int ecies_encrypt_with_priv(const uint8_t *msg, size_t msg_len,
                              const uint8_t priv_key[32],
                              uint8_t *out, size_t *out_len)
{
    if (!msg && msg_len > 0) return -1;
    if (!priv_key || !out || !out_len) return -1;

    uint8_t pub[32];
    x25519_public_key(pub, priv_key);

    uint8_t hash[32];
    blake2s(hash, pub, 32);

    uint8_t enc_key[16];
    memcpy(enc_key, hash, 16);

    if (getenv("PRIOTPS_PROOF_MODE")) {
        printf("[PROOF][ECIES_INTERMEDIATE]\nshared_secret_hash=");
        for(int i=0; i<32; i++) printf("%02x", hash[i]);
        printf("\n\n");
    }

    uint8_t nonce[16] = {0};
    if (read_random(nonce, 8) != 0) return -1;

    memcpy(out, nonce, 8);
    ascon_aead128_encrypt(out + 8, msg, msg_len, NULL, 0, nonce, enc_key);

    *out_len = 8 + msg_len + 16;
    return 0;
}

int ecies_decrypt_with_pub(const uint8_t *ct, size_t ct_len,
                             const uint8_t pub_key[32],
                             uint8_t *out, size_t *out_len)
{
    if (!ct || !pub_key || !out || !out_len) return -1;
    if (ct_len < 24) return -1;

    uint8_t hash[32];
    blake2s(hash, pub_key, 32);

    uint8_t enc_key[16];
    memcpy(enc_key, hash, 16);

    uint8_t nonce[16] = {0};
    memcpy(nonce, ct, 8);

    size_t enc_len = ct_len - 8;
    int res = ascon_aead128_decrypt(out, ct + 8, enc_len, NULL, 0, nonce, enc_key);
    if (res != 0) return -1;

    *out_len = enc_len - 16;
    return 0;
}

int ecies_encrypt_with_pub(const uint8_t *msg, size_t msg_len,
                             const uint8_t pub_key[32],
                             uint8_t *out, size_t *out_len)
{
    if (!msg && msg_len > 0) return -1;
    if (!pub_key || !out || !out_len) return -1;

    uint8_t hash[32];
    blake2s(hash, pub_key, 32);

    uint8_t enc_key[16];
    memcpy(enc_key, hash, 16);

    uint8_t nonce[16] = {0};
    if (read_random(nonce, 8) != 0) return -1;

    memcpy(out, nonce, 8);
    ascon_aead128_encrypt(out + 8, msg, msg_len, NULL, 0, nonce, enc_key);

    *out_len = 8 + msg_len + 16;
    return 0;
}

int ecies_decrypt_with_priv(const uint8_t *ct, size_t ct_len,
                              const uint8_t priv_key[32],
                              uint8_t *out, size_t *out_len)
{
    if (!ct || !priv_key || !out || !out_len) return -1;
    if (ct_len < 24) return -1;

    uint8_t pub[32];
    x25519_public_key(pub, priv_key);

    uint8_t hash[32];
    blake2s(hash, pub, 32);

    uint8_t enc_key[16];
    memcpy(enc_key, hash, 16);

    uint8_t nonce[16] = {0};
    memcpy(nonce, ct, 8);

    size_t enc_len = ct_len - 8;
    int res = ascon_aead128_decrypt(out, ct + 8, enc_len, NULL, 0, nonce, enc_key);
    if (res != 0) return -1;

    *out_len = enc_len - 16;
    return 0;
}

/* --------------------------------------------------------------------------
 * Phase 2 API
 * -------------------------------------------------------------------------- */

/**
 * @brief Encrypt to recipient's public key (Phase 2 unified API).
 * Delegates to ecies_encrypt_with_pub().
 */
int ecies_encrypt(const uint8_t *msg, size_t msg_len,
                   const uint8_t pub_key[32],
                   uint8_t *out, size_t *out_len)
{
    return ecies_encrypt_with_pub(msg, msg_len, pub_key, out, out_len);
}

/**
 * @brief Decrypt with private key (Phase 2 unified API).
 * Delegates to ecies_decrypt_with_priv().
 */
int ecies_decrypt(const uint8_t *ct, size_t ct_len,
                   const uint8_t priv_key[32],
                   uint8_t *out, size_t *out_len)
{
    return ecies_decrypt_with_priv(ct, ct_len, priv_key, out, out_len);
}
