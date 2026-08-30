#include "blake2s.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(void) {
    uint8_t out[BLAKE2S_OUTBYTES];
    int fail = 0;

    // Test 1: Empty string
    const uint8_t expected_empty[BLAKE2S_OUTBYTES] = {
        0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94,
        0xe1, 0x11, 0x21, 0xd0, 0x42, 0x35, 0x4a, 0x7c,
        0x1f, 0x55, 0xb6, 0x48, 0x2a, 0x15, 0xf3, 0xa5,
        0x26, 0xd9, 0xb2, 0xf1, 0x13, 0x37, 0x55, 0x19
    };
    blake2s(out, "", 0);
    if (memcmp(out, expected_empty, BLAKE2S_OUTBYTES) != 0) {
        printf("FAIL: Test 1 (Empty string)\n");
        fail = 1;
    }

    // Test 2: "abc"
    const uint8_t expected_abc[BLAKE2S_OUTBYTES] = {
        0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
        0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
        0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
        0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82
    };
    blake2s(out, "abc", 3);
    if (memcmp(out, expected_abc, BLAKE2S_OUTBYTES) != 0) {
        printf("FAIL: Test 2 (\"abc\")\n");
        fail = 1;
    }

    // Test 3: 64 bytes of zeros
    uint8_t zeros[64] = {0};
    blake2s(out, zeros, sizeof(zeros));
    // Verify it doesn't crash and gives some consistent output

    // Test 4: Streaming interface
    blake2s_state S;
    blake2s_init(&S);
    blake2s_update(&S, "a", 1);
    blake2s_update(&S, "b", 1);
    blake2s_update(&S, "c", 1);
    uint8_t out_stream[BLAKE2S_OUTBYTES];
    blake2s_final(&S, out_stream);
    if (memcmp(out_stream, expected_abc, BLAKE2S_OUTBYTES) != 0) {
        printf("FAIL: Test 4 (Streaming interface)\n");
        fail = 1;
    }

    if (fail) {
        printf("FAIL\n");
        return 1;
    } else {
        printf("PASS\n");
        return 0;
    }
}
