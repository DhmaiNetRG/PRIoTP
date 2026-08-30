#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../blake2s/blake2s.h"
#include "x25519.h"

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

static void hex2bin(uint8_t *out, const char *hex, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned int val = 0;
        sscanf(hex + 2 * i, "%2x", &val);
        out[i] = (uint8_t)val;
    }
}

static int all_zero(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) if (buf[i]) return 0;
    return 1;
}

int main(void) {
    uint8_t privA[32], pubA[32];
    uint8_t privB[32], pubB[32];

    /* Test 1: x25519_generate_keypair() returns 0 and non-zero keys */
    int ret = x25519_generate_keypair(privA, pubA);
    CHECK(ret == 0 && !all_zero(pubA, 32) && !all_zero(privA, 32),
          "x25519_generate_keypair produces non-zero keys");

    /* Test 2: x25519_public_from_private matches x25519_public_key */
    uint8_t pubA2[32], pubA3[32];
    x25519_public_from_private(pubA2, privA);
    x25519_public_key(pubA3, privA);
    CHECK(memcmp(pubA2, pubA, 32) == 0 && memcmp(pubA3, pubA, 32) == 0,
          "x25519_public_from_private matches x25519_public_key");

    /* Test 3: x25519_shared_secret matches x25519 (same args: shared, priv, peer_pub) */
    x25519_generate_keypair(privB, pubB);
    uint8_t secretAB[32], secretAB_raw[32];
    x25519_shared_secret(secretAB,     privA, pubB);
    x25519(secretAB_raw,               privA, pubB);
    CHECK(memcmp(secretAB, secretAB_raw, 32) == 0,
          "x25519_shared_secret matches x25519");

    /* Test 4: Diffie-Hellman: shared secret is symmetric */
    uint8_t secretBA[32];
    x25519_shared_secret(secretBA, privB, pubA);
    CHECK(memcmp(secretAB, secretBA, 32) == 0,
          "DH shared secret is symmetric: SS(privA,pubB) == SS(privB,pubA)");

    /* Test 5: RFC 7748 Section 6.1 — Alice public key vector */
    uint8_t rfc_priv[32], rfc_pub_expected[32], rfc_pub_actual[32];
    hex2bin(rfc_priv,
            "77076d0a7318a57d3c16c17251b26645"
            "df4c2f87ebc0992ab177fba51db92c2a", 32);
    hex2bin(rfc_pub_expected,
            "8520f0098930a754748b7ddcb43ef75a"
            "0dbf3a0d26381af4eba4a98eaa9b4e6a", 32);
    x25519_public_from_private(rfc_pub_actual, rfc_priv);
    CHECK(memcmp(rfc_pub_actual, rfc_pub_expected, 32) == 0,
          "RFC 7748 Section 6.1 Alice public key vector");

    /* Test 6: RFC 7748 — Bob public key vector */
    uint8_t bob_priv[32], bob_pub_expected[32], bob_pub_actual[32];
    hex2bin(bob_priv,
            "5dab087e624a8a4b79e17f8b83800ee6"
            "6f3bb1292618b6fd1c2f8b27ff88e0eb", 32);
    hex2bin(bob_pub_expected,
            "de9edb7d7b7dc1b4d35b61c2ece43537"
            "3f8343c85b78674dadfc7e146f882b4f", 32);
    x25519_public_from_private(bob_pub_actual, bob_priv);
    CHECK(memcmp(bob_pub_actual, bob_pub_expected, 32) == 0,
          "RFC 7748 Section 6.1 Bob public key vector");

    /* Test 7: RFC 7748 — shared secret vector */
    uint8_t rfc_shared_expected[32], rfc_shared_actual[32];
    hex2bin(rfc_shared_expected,
            "4a5d9d5ba4ce2de1728e3bf480350f25"
            "e07e21c947d19e3376f09b3c1e161742", 32);
    x25519_shared_secret(rfc_shared_actual, rfc_priv, bob_pub_expected);
    CHECK(memcmp(rfc_shared_actual, rfc_shared_expected, 32) == 0,
          "RFC 7748 Section 6.1 shared secret vector");

    /* Test 8: Different key pairs produce different shared secrets */
    uint8_t privC[32], pubC[32];
    x25519_generate_keypair(privC, pubC);
    uint8_t secretAC[32];
    x25519_shared_secret(secretAC, privA, pubC);
    CHECK(memcmp(secretAB, secretAC, 32) != 0,
          "Different peers produce different shared secrets");

    printf("\n=== X25519 Phase 2 Results ===\n");
    printf("Total passed: %d\nTotal failed: %d\n", tests_passed, tests_failed);
    return (tests_failed > 0) ? 1 : 0;
}
