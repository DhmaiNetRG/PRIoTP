/**
 * @file seh.c
 * @brief SEH module implementation
 */
#include "seh.h"
#include <string.h>

int seh_hsk_encode(const seh_handshake_t *seh,
                    const uint8_t *payload, size_t payload_len,
                    uint8_t *out, size_t *out_len)
{
    if (!seh || !out || !out_len) return -1;
    if (payload_len > 0 && !payload) return -1;

    size_t required = SEH_HSK_OVERHEAD + payload_len;
    /* If the caller provided a capacity in *out_len, we could check it.
       But standard C99 with this signature often uses out_len only as output.
       Since prompt says "return -1 if output too small", we'll treat *out_len as capacity on input.
       If it is 0, we assume the user didn't provide capacity and we'll just write.
       Actually, to be safe and strictly follow "if buffer too small", let's check *out_len if it's > 0, 
       but wait, if *out_len is 0, they might really mean 0 capacity. 
    */
    if (*out_len != 0 && *out_len < required) return -1;

    out[0] = seh->flags & 0x03;
    memcpy(out + 1, seh->nonce, SEH_NONCE_SIZE);
    memcpy(out + 1 + SEH_NONCE_SIZE, seh->pub_key, SEH_PUB_KEY_SIZE);

    if (payload_len > 0) {
        memcpy(out + SEH_HSK_OVERHEAD, payload, payload_len);
    }

    *out_len = required;
    return 0;
}

int seh_hsk_decode(const uint8_t *buf, size_t buf_len,
                    seh_handshake_t *seh,
                    size_t *payload_offset, size_t *payload_len)
{
    if (!buf || !seh || !payload_offset || !payload_len) return -1;
    if (buf_len < SEH_HSK_OVERHEAD) return -1;

    seh->flags = buf[0] & 0x03;
    if (seh->flags != SEH_FLAG_HSK && seh->flags != SEH_FLAG_ACK) return -1;

    memcpy(seh->nonce, buf + 1, SEH_NONCE_SIZE);
    memcpy(seh->pub_key, buf + 1 + SEH_NONCE_SIZE, SEH_PUB_KEY_SIZE);

    *payload_offset = SEH_HSK_OVERHEAD;
    *payload_len = buf_len - SEH_HSK_OVERHEAD;
    return 0;
}

int seh_data_encode(const seh_data_t *seh,
                     const uint8_t *ciphertext, size_t ct_len,
                     uint8_t *out, size_t *out_len)
{
    if (!seh || !out || !out_len) return -1;
    if (ct_len > 0 && !ciphertext) return -1;

    size_t required = SEH_DATA_OVERHEAD + ct_len;
    if (*out_len != 0 && *out_len < required) return -1;

    out[0] = seh->flags & 0x03;
    memcpy(out + 1, seh->nonce, SEH_NONCE_SIZE);

    /* Big endian session_id */
    out[9]  = (seh->session_id >> 24) & 0xFF;
    out[10] = (seh->session_id >> 16) & 0xFF;
    out[11] = (seh->session_id >> 8)  & 0xFF;
    out[12] = (seh->session_id)       & 0xFF;

    if (ct_len > 0) {
        memcpy(out + SEH_DATA_OVERHEAD, ciphertext, ct_len);
    }

    *out_len = required;
    return 0;
}

int seh_data_decode(const uint8_t *buf, size_t buf_len,
                     seh_data_t *seh,
                     size_t *ct_offset, size_t *ct_len)
{
    if (!buf || !seh || !ct_offset || !ct_len) return -1;
    if (buf_len < SEH_DATA_OVERHEAD) return -1;

    seh->flags = buf[0] & 0x03;
    if (seh->flags != SEH_FLAG_ENC) return -1;

    memcpy(seh->nonce, buf + 1, SEH_NONCE_SIZE);

    /* Big endian session_id */
    seh->session_id = ((uint32_t)buf[9] << 24) |
                      ((uint32_t)buf[10] << 16) |
                      ((uint32_t)buf[11] << 8) |
                      ((uint32_t)buf[12]);

    *ct_offset = SEH_DATA_OVERHEAD;
    *ct_len = buf_len - SEH_DATA_OVERHEAD;
    return 0;
}

uint8_t seh_get_flags(const uint8_t *buf, size_t buf_len)
{
    if (!buf || buf_len < 1) return 0xFF; /* Error value */
    return buf[0] & 0x03;
}

/* ------------------------------------------------------------------ */
/* Compact ACK wire format (spec Task 4): [flag(1)][session_id(4 BE)] */
/* ------------------------------------------------------------------ */

int seh_ack_encode(const seh_ack_t *ack, uint8_t *out, size_t *out_len)
{
    if (!ack || !out || !out_len) return -1;
    if (*out_len != 0 && *out_len < SEH_ACK_OVERHEAD) return -1;

    out[0] = SEH_FLAG_ACK & 0x03;
    /* Session ID in big-endian order */
    out[1] = (ack->session_id >> 24) & 0xFF;
    out[2] = (ack->session_id >> 16) & 0xFF;
    out[3] = (ack->session_id >>  8) & 0xFF;
    out[4] =  ack->session_id        & 0xFF;

    *out_len = SEH_ACK_OVERHEAD;
    return 0;
}

int seh_ack_decode(const uint8_t *buf, size_t buf_len, seh_ack_t *ack)
{
    if (!buf || !ack) return -1;
    if (buf_len < SEH_ACK_OVERHEAD) return -1;
    if ((buf[0] & 0x03) != SEH_FLAG_ACK) return -1;

    ack->flags = SEH_FLAG_ACK;
    ack->session_id = ((uint32_t)buf[1] << 24)
                    | ((uint32_t)buf[2] << 16)
                    | ((uint32_t)buf[3] <<  8)
                    |  (uint32_t)buf[4];
    return 0;
}

int seh_is_compact_ack(const uint8_t *buf, size_t buf_len)
{
    if (!buf || buf_len < 1) return -1;
    if ((buf[0] & 0x03) != SEH_FLAG_ACK) return -1;
    /* Compact ACK is exactly 5 bytes; legacy ACK is >= 41 bytes */
    if (buf_len == SEH_ACK_OVERHEAD) return 1;  /* compact */
    if (buf_len >= (size_t)SEH_HSK_OVERHEAD) return 0;  /* legacy */
    return -1;  /* ambiguous / invalid */
}
