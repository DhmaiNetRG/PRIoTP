#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ascon.h"

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
    uint8_t key[ASCON_KEY_LEN]   = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint8_t nonce[ASCON_NONCE_LEN] = {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

    /* Plaintext buffers */
    uint8_t pt32[32];
    for (int i = 0; i < 32; i++) pt32[i] = (uint8_t)i;

    uint8_t pt64[64];
    for (int i = 0; i < 64; i++) pt64[i] = (uint8_t)(i * 2);

    uint8_t aad16[16] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x11,
                          0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};

    /* Output buffers: plaintext + ASCON_TAG_LEN */
    uint8_t ct[128];
    uint8_t dec[128];
    uint8_t ct2[128];

    /* Test 1: 32-byte plaintext, no AAD */
    ascon_encrypt(ct, pt32, 32, NULL, 0, nonce, key);
    int ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, NULL, 0, nonce, key);
    CHECK(ret == 0 && memcmp(pt32, dec, 32) == 0,
          "ascon round-trip 32-byte no AAD");

    /* Test 2: 0-byte plaintext (tag only output) */
    ascon_encrypt(ct, NULL, 0, NULL, 0, nonce, key);
    ret = ascon_decrypt(dec, ct, ASCON_TAG_LEN, NULL, 0, nonce, key);
    CHECK(ret == 0, "ascon round-trip 0-byte plaintext");

    /* Test 3: 64-byte plaintext + 16-byte AAD */
    ascon_encrypt(ct, pt64, 64, aad16, 16, nonce, key);
    ret = ascon_decrypt(dec, ct, 64 + ASCON_TAG_LEN, aad16, 16, nonce, key);
    CHECK(ret == 0 && memcmp(pt64, dec, 64) == 0,
          "ascon round-trip 64-byte + 16-byte AAD");

    /* Test 4: ascon_encrypt matches ascon_aead128_encrypt */
    ascon_aead128_encrypt(ct2, pt32, 32, aad16, 16, nonce, key);
    ascon_encrypt(ct,         pt32, 32, aad16, 16, nonce, key);
    CHECK(memcmp(ct, ct2, 32 + ASCON_TAG_LEN) == 0,
          "ascon_encrypt output matches ascon_aead128_encrypt");

    /* Test 5: ascon_decrypt matches ascon_aead128_decrypt */
    uint8_t dec2[128];
    ret = ascon_aead128_decrypt(dec,  ct, 32 + ASCON_TAG_LEN, aad16, 16, nonce, key);
    int ret2 = ascon_decrypt(   dec2, ct, 32 + ASCON_TAG_LEN, aad16, 16, nonce, key);
    CHECK(ret == 0 && ret2 == 0 && memcmp(dec, dec2, 32) == 0,
          "ascon_decrypt output matches ascon_aead128_decrypt");

    /* Prepare a valid ciphertext for tamper tests */
    ascon_encrypt(ct, pt32, 32, aad16, 16, nonce, key);

    /* Test 6: Tampered ciphertext fails authentication */
    ct[0] ^= 0x01;
    ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, aad16, 16, nonce, key);
    CHECK(ret == -1, "Tampered ciphertext body fails decrypt");
    ct[0] ^= 0x01;  /* Restore */

    /* Test 7: Wrong key fails authentication */
    uint8_t wrong_key[ASCON_KEY_LEN];
    memcpy(wrong_key, key, ASCON_KEY_LEN);
    wrong_key[7] ^= 0x01;
    ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, aad16, 16, nonce, wrong_key);
    CHECK(ret == -1, "Wrong key fails decrypt");

    /* Test 8: Wrong nonce fails authentication */
    uint8_t wrong_nonce[ASCON_NONCE_LEN];
    memcpy(wrong_nonce, nonce, ASCON_NONCE_LEN);
    wrong_nonce[0] ^= 0x01;
    ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, aad16, 16, wrong_nonce, key);
    CHECK(ret == -1, "Wrong nonce fails decrypt");

    /* Test 9: Tampered AAD fails authentication */
    uint8_t wrong_aad[16];
    memcpy(wrong_aad, aad16, 16);
    wrong_aad[0] ^= 0x01;
    ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, wrong_aad, 16, nonce, key);
    CHECK(ret == -1, "Tampered AAD fails decrypt");

    /* Test 10: Tampered tag fails authentication */
    /* Tag is at ct[32..47] */
    ct[32] ^= 0xFF;
    ret = ascon_decrypt(dec, ct, 32 + ASCON_TAG_LEN, aad16, 16, nonce, key);
    CHECK(ret == -1, "Tampered authentication tag fails decrypt");
    ct[32] ^= 0xFF;  /* Restore */

    printf("\n=== ASCON-AEAD128 Phase 2 Results ===\n");
    printf("Total passed: %d\nTotal failed: %d\n", tests_passed, tests_failed);
    return (tests_failed > 0) ? 1 : 0;
}
