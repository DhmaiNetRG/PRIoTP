#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../x25519/x25519.h"
#include "ecies.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, name) do { \
    if (cond) { \
        printf("[PASS] %s\n", name); \
        tests_passed++; \
    } else { \
        printf("[FAIL] %s\n", name); \
        tests_failed++; \
    } \
} while(0)

int main(void) {
    /* Set up a key pair for the recipient */
    uint8_t rec_priv[32], rec_pub[32];
    /* Use fixed private key for reproducibility */
    for (int i = 0; i < 32; i++) rec_priv[i] = (uint8_t)(i + 1);
    x25519_public_key(rec_pub, rec_priv);

    uint8_t pt16[16] = "0123456789abcde";
    uint8_t pt100[100];
    memset(pt100, 0x42, 100);
    uint8_t ct[256];
    uint8_t dec[256];
    size_t ct_len, dec_len;

    /* Test 1: ecies_encrypt/ecies_decrypt round-trip — 16 bytes */
    int ret = ecies_encrypt(pt16, 16, rec_pub, ct, &ct_len);
    CHECK(ret == 0 && ct_len == 16 + ECIES_OVERHEAD,
          "ecies_encrypt 16-byte succeeds with correct output length");
    ret = ecies_decrypt(ct, ct_len, rec_priv, dec, &dec_len);
    CHECK(ret == 0 && dec_len == 16 && memcmp(pt16, dec, 16) == 0,
          "ecies_decrypt 16-byte round-trip");

    /* Test 2: ecies_encrypt/ecies_decrypt round-trip — 100 bytes */
    ret = ecies_encrypt(pt100, 100, rec_pub, ct, &ct_len);
    CHECK(ret == 0 && ct_len == 100 + ECIES_OVERHEAD,
          "ecies_encrypt 100-byte succeeds with correct output length");
    ret = ecies_decrypt(ct, ct_len, rec_priv, dec, &dec_len);
    CHECK(ret == 0 && dec_len == 100 && memcmp(pt100, dec, 100) == 0,
          "ecies_decrypt 100-byte round-trip");

    /* Test 3: ecies_encrypt overhead same as ecies_encrypt_with_pub */
    size_t ct2_len;
    uint8_t ct2[256];
    ret = ecies_encrypt_with_pub(pt16, 16, rec_pub, ct2, &ct2_len);
    CHECK(ret == 0 && ct2_len == 16 + ECIES_OVERHEAD,
          "ecies_encrypt_with_pub gives same overhead as ecies_encrypt");

    /* Test 4: Tampered ciphertext fails authentication */
    /* First get a fresh valid ciphertext */
    ecies_encrypt(pt16, 16, rec_pub, ct, &ct_len);
    ct[10] ^= 0xFF;  /* Corrupt ciphertext body */
    ret = ecies_decrypt(ct, ct_len, rec_priv, dec, &dec_len);
    CHECK(ret == -1, "Tampered ciphertext fails decrypt");
    ct[10] ^= 0xFF;  /* Restore */

    /* Test 5: Wrong private key fails authentication */
    uint8_t wrong_priv[32];
    memset(wrong_priv, 0x11, 32);
    ret = ecies_decrypt(ct, ct_len, wrong_priv, dec, &dec_len);
    CHECK(ret == -1, "Wrong private key fails decrypt");

    /* Test 6: Two calls to ecies_encrypt produce different nonces (random) */
    uint8_t ct3[256];
    size_t ct3_len;
    ecies_encrypt(pt16, 16, rec_pub, ct,  &ct_len);
    ecies_encrypt(pt16, 16, rec_pub, ct3, &ct3_len);
    CHECK(memcmp(ct, ct3, ct_len) != 0,
          "ecies_encrypt produces different ciphertexts (random nonce)");
    /* Both should still decrypt correctly */
    ret = ecies_decrypt(ct,  ct_len,  rec_priv, dec, &dec_len);
    CHECK(ret == 0 && memcmp(pt16, dec, 16) == 0, "First  random-nonce CT decrypts");
    ret = ecies_decrypt(ct3, ct3_len, rec_priv, dec, &dec_len);
    CHECK(ret == 0 && memcmp(pt16, dec, 16) == 0, "Second random-nonce CT decrypts");

    /* Test 7: Empty message — ecies_encrypt output is exactly ECIES_OVERHEAD bytes */
    uint8_t empty_ct[ECIES_OVERHEAD + 4];
    size_t empty_ct_len;
    ret = ecies_encrypt(NULL, 0, rec_pub, empty_ct, &empty_ct_len);
    CHECK(ret == 0 && empty_ct_len == (size_t)ECIES_OVERHEAD,
          "Empty message output is exactly ECIES_OVERHEAD bytes");
    ret = ecies_decrypt(empty_ct, empty_ct_len, rec_priv, dec, &dec_len);
    CHECK(ret == 0 && dec_len == 0, "Empty message decrypts to 0 bytes");

    /* Test 8: ecies_encrypt_with_pub decrypts via ecies_decrypt_with_priv */
    ecies_encrypt_with_pub(pt16, 16, rec_pub, ct2, &ct2_len);
    ret = ecies_decrypt_with_priv(ct2, ct2_len, rec_priv, dec, &dec_len);
    CHECK(ret == 0 && dec_len == 16 && memcmp(pt16, dec, 16) == 0,
          "ecies_encrypt_with_pub / ecies_decrypt_with_priv round-trip");

    printf("\n=== ECIES Phase 2 Results ===\n");
    printf("Total passed: %d\nTotal failed: %d\n", tests_passed, tests_failed);
    return (tests_failed > 0) ? 1 : 0;
}
