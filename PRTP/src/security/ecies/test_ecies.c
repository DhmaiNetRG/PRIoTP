/**
 * @file test_ecies.c
 * @brief Tests for ECIES module
 */
#include "ecies.h"
#include "../x25519/x25519.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    uint8_t priv[32] = {1};
    uint8_t pub[32];
    x25519_public_key(pub, priv);

    const uint8_t msg[] = "Hello IoT World!";
    size_t msg_len = strlen((const char *)msg);
    uint8_t ct[128];
    size_t ct_len = 0;
    uint8_t out[128];
    size_t out_len = 0;

    /* Test 1: encrypt_with_priv then decrypt_with_pub */
    assert(ecies_encrypt_with_priv(msg, msg_len, priv, ct, &ct_len) == 0);
    assert(ct_len == msg_len + ECIES_OVERHEAD);

    assert(ecies_decrypt_with_pub(ct, ct_len, pub, out, &out_len) == 0);
    assert(out_len == msg_len);
    assert(memcmp(msg, out, msg_len) == 0);

    /* Test 2: encrypt_with_pub then decrypt_with_priv */
    memset(ct, 0, sizeof(ct));
    memset(out, 0, sizeof(out));
    ct_len = 0;
    out_len = 0;

    assert(ecies_encrypt_with_pub(msg, msg_len, pub, ct, &ct_len) == 0);
    assert(ct_len == msg_len + ECIES_OVERHEAD);

    assert(ecies_decrypt_with_priv(ct, ct_len, priv, out, &out_len) == 0);
    assert(out_len == msg_len);
    assert(memcmp(msg, out, msg_len) == 0);

    /* Test 3: Wrong key fails authentication */
    uint8_t wrong_pub[32] = {0}; wrong_pub[1] = 2;
    assert(ecies_decrypt_with_pub(ct, ct_len, wrong_pub, out, &out_len) != 0);

    uint8_t wrong_priv[32] = {0}; wrong_priv[1] = 2;
    assert(ecies_decrypt_with_priv(ct, ct_len, wrong_priv, out, &out_len) != 0);

    /* Test 4: Tampered ciphertext fails authentication */
    ct[10] ^= 0x01; /* tamper with ciphertext */
    assert(ecies_decrypt_with_priv(ct, ct_len, priv, out, &out_len) != 0);

    printf("All ECIES tests passed!\n");
    return 0;
}
