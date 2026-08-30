#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "blake2s.h"

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

int main(void) {
    uint8_t hash[32];
    uint8_t hash2[32];
    uint8_t expected[32];
    blake2s_state st;

    /* Test 1: blake2s_hash("") == RFC 7693 vector */
    blake2s_hash(hash, "", 0);
    hex2bin(expected, "69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9", 32);
    CHECK(memcmp(hash, expected, 32) == 0, "blake2s_hash empty string (RFC 7693)");

    /* Test 2: blake2s_hash("abc") == RFC 7693 vector */
    blake2s_hash(hash, "abc", 3);
    hex2bin(expected, "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982", 32);
    CHECK(memcmp(hash, expected, 32) == 0, "blake2s_hash abc (RFC 7693)");

    /* Test 3: blake2s_hash == blake2s one-shot for same input */
    blake2s(hash2, "abc", 3);
    CHECK(memcmp(hash, hash2, 32) == 0, "blake2s_hash matches blake2s one-shot");

    /* Test 4: Streaming init/update/final matches blake2s_hash */
    blake2s_init(&st);
    blake2s_update(&st, "abc", 3);
    blake2s_final(&st, hash2);
    CHECK(memcmp(hash, hash2, 32) == 0, "Streaming init/update/final matches blake2s_hash");

    /* Test 5: Two consecutive updates give same result as single update */
    blake2s_init(&st);
    blake2s_update(&st, "a", 1);
    blake2s_update(&st, "bc", 2);
    blake2s_final(&st, hash2);
    CHECK(memcmp(hash, hash2, 32) == 0, "Consecutive updates match single update");

    /* Test 6: Zero-length input streaming == zero-length one-shot */
    blake2s_hash(hash, "", 0);
    blake2s_init(&st);
    blake2s_update(&st, "", 0);
    blake2s_final(&st, hash2);
    CHECK(memcmp(hash, hash2, 32) == 0, "Zero-length streaming matches one-shot");

    /* Test 7: Different inputs produce different hashes */
    blake2s_hash(hash,  "hello", 5);
    blake2s_hash(hash2, "world", 5);
    CHECK(memcmp(hash, hash2, 32) != 0, "Different inputs produce different hashes");

    /* Test 8: blake2s_hash is deterministic */
    blake2s_hash(hash,  "determinism", 11);
    blake2s_hash(hash2, "determinism", 11);
    CHECK(memcmp(hash, hash2, 32) == 0, "blake2s_hash is deterministic");

    printf("\n=== BLAKE2s Phase 2 Results ===\n");
    printf("Total passed: %d\nTotal failed: %d\n", tests_passed, tests_failed);
    return (tests_failed > 0) ? 1 : 0;
}
