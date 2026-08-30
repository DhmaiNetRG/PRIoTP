/**
 * @file test_seh.c
 * @brief Tests for SEH module
 */
#include "seh.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    uint8_t buf[128];
    size_t out_len;

    /* Test 1: Encode HSK SEH then decode */
    seh_handshake_t hsk_in = {
        .flags = SEH_FLAG_HSK,
        .nonce = {1,2,3,4,5,6,7,8},
        .pub_key = {
            0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
            0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
        }
    };
    uint8_t payload[] = "SCT_DATA";
    out_len = sizeof(buf);
    assert(seh_hsk_encode(&hsk_in, payload, sizeof(payload), buf, &out_len) == 0);
    assert(out_len == SEH_HSK_OVERHEAD + sizeof(payload));

    seh_handshake_t hsk_out;
    size_t p_off, p_len;
    assert(seh_hsk_decode(buf, out_len, &hsk_out, &p_off, &p_len) == 0);
    assert(hsk_out.flags == SEH_FLAG_HSK);
    assert(memcmp(hsk_out.nonce, hsk_in.nonce, SEH_NONCE_SIZE) == 0);
    assert(memcmp(hsk_out.pub_key, hsk_in.pub_key, SEH_PUB_KEY_SIZE) == 0);
    assert(p_off == SEH_HSK_OVERHEAD);
    assert(p_len == sizeof(payload));
    assert(memcmp(buf + p_off, payload, p_len) == 0);

    /* Test 2: Encode Data SEH then decode & Test 4: session_id big-endian */
    seh_data_t data_in = {
        .flags = SEH_FLAG_ENC,
        .nonce = {8,7,6,5,4,3,2,1},
        .session_id = 0x12345678
    };
    uint8_t ct[] = "ENCRYPTED_STUFF";
    out_len = sizeof(buf);
    assert(seh_data_encode(&data_in, ct, sizeof(ct), buf, &out_len) == 0);
    assert(out_len == SEH_DATA_OVERHEAD + sizeof(ct));

    /* Verify big endian session_id manually */
    assert(buf[9] == 0x12);
    assert(buf[10] == 0x34);
    assert(buf[11] == 0x56);
    assert(buf[12] == 0x78);

    seh_data_t data_out;
    assert(seh_data_decode(buf, out_len, &data_out, &p_off, &p_len) == 0);
    assert(data_out.flags == SEH_FLAG_ENC);
    assert(memcmp(data_out.nonce, data_in.nonce, SEH_NONCE_SIZE) == 0);
    assert(data_out.session_id == 0x12345678);
    assert(p_off == SEH_DATA_OVERHEAD);
    assert(p_len == sizeof(ct));
    assert(memcmp(buf + p_off, ct, p_len) == 0);

    /* Test 3: Flags byte correctly set */
    assert(seh_get_flags(buf, out_len) == SEH_FLAG_ENC);
    
    /* Test 5: Buffer too small returns -1 */
    out_len = SEH_HSK_OVERHEAD - 1; /* For decode */
    assert(seh_hsk_decode(buf, out_len, &hsk_out, &p_off, &p_len) == -1);

    out_len = SEH_DATA_OVERHEAD - 1;
    assert(seh_data_decode(buf, out_len, &data_out, &p_off, &p_len) == -1);

    /* Test encode buffer too small */
    out_len = SEH_HSK_OVERHEAD - 1;
    assert(seh_hsk_encode(&hsk_in, NULL, 0, buf, &out_len) == -1);

    out_len = SEH_DATA_OVERHEAD - 1;
    assert(seh_data_encode(&data_in, NULL, 0, buf, &out_len) == -1);

    printf("All SEH tests passed!\n");
    return 0;
}
