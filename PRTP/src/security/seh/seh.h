#ifndef SEH_H
#define SEH_H
#include <stdint.h>
#include <stddef.h>

/* Flag values (stored in low 2 bits of flags byte) */
#define SEH_FLAG_HSK  0
#define SEH_FLAG_ENC  1
#define SEH_FLAG_ACK  2

#define SEH_PUB_KEY_SIZE   32
#define SEH_NONCE_SIZE      8
#define SEH_SESSION_ID_SIZE 4

/* Overhead sizes (bytes) */
#define SEH_HSK_OVERHEAD  41  /* 1 flag + 8 nonce + 32 pubkey */
#define SEH_DATA_OVERHEAD 13  /* 1 flag + 8 nonce + 4 session_id */
#define SEH_ACK_OVERHEAD   5  /* 1 flag + 4 session_id (spec-compliant ACK) */

/** In-memory Handshake SEH */
typedef struct {
    uint8_t  flags;                  /* SEH_FLAG_HSK or SEH_FLAG_ACK */
    uint8_t  nonce[SEH_NONCE_SIZE];  /* 8-byte nonce */
    uint8_t  pub_key[SEH_PUB_KEY_SIZE]; /* sender public key */
} seh_handshake_t;

/** In-memory Data SEH */
typedef struct {
    uint8_t  flags;                  /* SEH_FLAG_ENC */
    uint8_t  nonce[SEH_NONCE_SIZE];  /* 8-byte nonce */
    uint32_t session_id;             /* session identifier */
} seh_data_t;

/**
 * @brief In-memory spec-compliant ACK SEH.
 *
 * Wire format: [flags(1)] [session_id(4 BE)] — total 5 bytes.
 * This is the compact ACK defined by the protocol specification.
 */
typedef struct {
    uint8_t  flags;       /* SEH_FLAG_ACK */
    uint32_t session_id;  /* session being acknowledged */
} seh_ack_t;

/**
 * @brief Serialize Handshake SEH + payload into wire buffer.
 * Output: [flags(1)] [nonce(8)] [pub_key(32)] [payload(payload_len)]
 * @param seh       Handshake SEH fields
 * @param payload   Payload bytes (SCT or empty for ACK)
 * @param payload_len Payload length
 * @param out       Output buffer
 * @param out_len   Set to total bytes written
 * @return 0 on success, -1 if output too small
 */
int seh_hsk_encode(const seh_handshake_t *seh,
                    const uint8_t *payload, size_t payload_len,
                    uint8_t *out, size_t *out_len);

/**
 * @brief Parse Handshake SEH from wire buffer.
 * @param buf     Wire buffer
 * @param buf_len Buffer length
 * @param seh     Output SEH fields
 * @param payload_offset Set to byte offset where payload starts
 * @param payload_len    Set to payload length
 * @return 0 on success, -1 on error
 */
int seh_hsk_decode(const uint8_t *buf, size_t buf_len,
                    seh_handshake_t *seh,
                    size_t *payload_offset, size_t *payload_len);

/**
 * @brief Serialize Data SEH + ciphertext into wire buffer.
 * Output: [flags(1)] [nonce(8)] [session_id(4 BE)] [ciphertext(ct_len)]
 */
int seh_data_encode(const seh_data_t *seh,
                     const uint8_t *ciphertext, size_t ct_len,
                     uint8_t *out, size_t *out_len);

/**
 * @brief Parse Data SEH from wire buffer.
 */
int seh_data_decode(const uint8_t *buf, size_t buf_len,
                     seh_data_t *seh,
                     size_t *ct_offset, size_t *ct_len);

/**
 * @brief Serialize a compact ACK SEH into wire buffer.
 * Output: [flags(1)] [session_id(4 BE)] — total SEH_ACK_OVERHEAD bytes.
 *
 * This is the spec-compliant ACK format (Task 4).
 *
 * @param ack     ACK SEH fields
 * @param out     Output buffer (must be >= SEH_ACK_OVERHEAD bytes)
 * @param out_len Set to SEH_ACK_OVERHEAD on success
 * @return 0 on success, -1 on error
 */
int seh_ack_encode(const seh_ack_t *ack, uint8_t *out, size_t *out_len);

/**
 * @brief Parse a compact ACK SEH from wire buffer.
 *
 * Accepts only the 5-byte compact format: [flag(1)][session_id(4 BE)].
 * Returns -1 if the buffer is not a valid compact ACK.
 *
 * @param buf     Wire buffer
 * @param buf_len Buffer length (must be >= SEH_ACK_OVERHEAD)
 * @param ack     Output ACK SEH fields
 * @return 0 on success, -1 on error
 */
int seh_ack_decode(const uint8_t *buf, size_t buf_len, seh_ack_t *ack);

/**
 * @brief Detect whether a wire buffer is a compact ACK (5-byte format).
 *
 * Returns 1 if buf[0] == SEH_FLAG_ACK and buf_len == SEH_ACK_OVERHEAD.
 * Returns 0 if it looks like a legacy ACK (41-byte HSK-style).
 * Returns -1 if the buffer cannot be either.
 */
int seh_is_compact_ack(const uint8_t *buf, size_t buf_len);

/**
 * @brief Read the flags byte from a wire buffer without full decode.
 */
uint8_t seh_get_flags(const uint8_t *buf, size_t buf_len);

#endif /* SEH_H */

