/**
 * @file handshake.c
 * @brief PRIoTPS secure handshake protocol implementation.
 *
 * Implements the 7-step hybrid handshake using double-ECIES SCT delivery:
 *
 *  Client → Server : SEH_HSK + PubB                     (Step 1)
 *  Server → Client : SEH_HSK + PubA + SCT               (Step 4)
 *  Client → Server : SEH_ACK                            (Step 6)
 *  Server marks  session->state = SESSION_ESTABLISHED   (Step 7)
 *
 * Structured log events emitted:
 *   [HSK] HandshakeStart
 *   [HSK] HandshakeResponse
 *   [HSK] HandshakeAck
 *   [HSK] HandshakeSuccess
 *   [HSK] HandshakeFailure
 *   [HSK] SessionCreated
 *   [HSK] SessionEstablished
 */
#include "handshake.h"
#include "../seh/seh.h"
#include "../ecies/ecies.h"
#include "../identity/identity.h"
#include "../session/session.h"
#include "../../ascon.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Internal logging helpers (spec Task 5 log format)                  */
/* ------------------------------------------------------------------ */

/* Emit a structured handshake log event. tag must be one of the spec
 * tags: HSK_START, HSK_RESPONSE, HSK_ACK, HSK_SUCCESS, HSK_FAILURE,
 * SESSION_CREATED, SESSION_ESTABLISHED, SESSION_EXPIRED.             */
static void hsk_log(const char *tag, uint32_t session_id, const char *detail) {
    if (detail && detail[0]) {
        fprintf(stderr, "[%s] session=%u %s\n", tag, session_id, detail);
    } else {
        fprintf(stderr, "[%s] session=%u\n", tag, session_id);
    }
}

static void hsk_log_failure(const char *reason) {
    fprintf(stderr, "[HSK_FAILURE] reason=%s\n", reason ? reason : "unknown");
}

/* ------------------------------------------------------------------ */
/* Utility: fill nonce from /dev/urandom (falls back to fixed bytes)  */
/* ------------------------------------------------------------------ */

static void fill_nonce(uint8_t *nonce, size_t len, uint8_t fallback_fill) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        fread(nonce, 1, len, f);
        fclose(f);
    } else {
        memset(nonce, fallback_fill, len);
    }
}

/* ------------------------------------------------------------------ */
/* Server Side                                                         */
/* ------------------------------------------------------------------ */

int handshake_server_build_response(
    const priotps_identity_t *server_id,
    const uint8_t client_pub[32],
    priotps_session_table_t *table,
    uint8_t *out_pkt, size_t *out_pkt_len,
    priotps_session_t **session_out)
{
    if (!server_id || !client_pub || !table || !out_pkt || !out_pkt_len) return -1;

    /* FIX 3: Validate Public Key Before Session Creation */
    int is_zero = 1;
    for (int i = 0; i < 32; i++) {
        if (client_pub[i] != 0) {
            is_zero = 0;
            break;
        }
    }
    if (is_zero) {
        hsk_log_failure("Invalid client public key (all zeros)");
        return -1;
    }

    hsk_log("HSK_START", 0, "server building response");

    priotps_session_t *session = session_create(table, client_pub);
    if (!session) {
        hsk_log_failure("session_create failed (table full)");
        return -1;
    }

    /* Print required log directly to stdout/stderr */
    fprintf(stderr, "PRIoTPS: Session created\n");
    hsk_log("SESSION_CREATED", session->session_id, NULL);

    /*
     * Inner payload: [session_id (4 bytes LE)] [session_key (16 bytes)]
     * Total: HANDSHAKE_INNER_PAYLOAD_SIZE = 20 bytes.
     */
    uint8_t inner_plain[HANDSHAKE_INNER_PAYLOAD_SIZE];
    inner_plain[0] = (uint8_t)( session->session_id        & 0xFF);
    inner_plain[1] = (uint8_t)((session->session_id >>  8) & 0xFF);
    inner_plain[2] = (uint8_t)((session->session_id >> 16) & 0xFF);
    inner_plain[3] = (uint8_t)((session->session_id >> 24) & 0xFF);
    memcpy(inner_plain + 4, session->session_key, SESSION_KEY_SIZE);

    /* CipherText_1 = ECIES_Encrypt(SessionKey, ServerPrivateKey) */
    uint8_t ct1[256];
    size_t ct1_len = 0;
    if (ecies_encrypt_with_priv(inner_plain, HANDSHAKE_INNER_PAYLOAD_SIZE,
                                 server_id->private_key, ct1, &ct1_len) != 0) {
        hsk_log_failure("ECIES inner encrypt failed");
        session_delete(session);
        return -1;
    }

    /* SCT = ECIES_Encrypt(CipherText_1, ClientPublicKey) */
    uint8_t sct[HANDSHAKE_MAX_SCT_SIZE];
    size_t sct_len = 0;
    if (ecies_encrypt_with_pub(ct1, ct1_len, client_pub, sct, &sct_len) != 0) {
        hsk_log_failure("ECIES outer encrypt failed");
        session_delete(session);
        return -1;
    }

    fprintf(stderr, "PRIoTPS: SCT generated\n");

    /* Build SEH_HSK wire packet: [flags][nonce(8)][server_pub(32)][SCT] */
    seh_handshake_t seh;
    memset(&seh, 0, sizeof(seh));
    seh.flags = SEH_FLAG_HSK;
    fill_nonce(seh.nonce, sizeof(seh.nonce), 0x11);
    memcpy(seh.pub_key, server_id->public_key, 32);

    if (seh_hsk_encode(&seh, sct, sct_len, out_pkt, out_pkt_len) != 0) {
        hsk_log_failure("seh_hsk_encode failed");
        session_delete(session);
        return -1;
    }

    hsk_log("HSK_RESPONSE", session->session_id, "SCT packet built");

    if (session_out) *session_out = session;
    return 0;
}


/* ------------------------------------------------------------------ */
/* Client Side                                                         */
/* ------------------------------------------------------------------ */

int handshake_client_process_response(

    const priotps_identity_t *client_id,
    const uint8_t *pkt, size_t pkt_len,
    uint8_t session_key_out[16],
    uint8_t server_pub_out[32],
    uint32_t *session_id_out)
{
    if (!client_id || !pkt || !session_key_out || !server_pub_out) return -1;

    seh_handshake_t seh;
    size_t payload_off = 0;
    size_t payload_len = 0;

    if (seh_hsk_decode(pkt, pkt_len, &seh, &payload_off, &payload_len) != 0) {
        fprintf(stderr, "HSK_FAIL:\n    Invalid SCT\n");
        hsk_log_failure("seh_hsk_decode failed");
        return -1;
    }

    memcpy(server_pub_out, seh.pub_key, 32);

    /* Step 5a: SCT → CipherText_1 using client private key */
    uint8_t ct1[256];
    size_t ct1_len = 0;
    if (ecies_decrypt_with_priv(pkt + payload_off, payload_len,
                                 client_id->private_key, ct1, &ct1_len) != 0) {
        fprintf(stderr, "HSK_FAIL:\n    ECIES outer decrypt failed\n");
        hsk_log_failure("ECIES outer decrypt failed");
        return -1;
    }

    /* Step 5b: CipherText_1 → inner plaintext using server public key */
    uint8_t inner_plain[HANDSHAKE_INNER_PAYLOAD_SIZE];
    size_t inner_len = 0;
    if (ecies_decrypt_with_pub(ct1, ct1_len, seh.pub_key, inner_plain, &inner_len) != 0) {
        fprintf(stderr, "HSK_FAIL:\n    ECIES inner decrypt failed\n");
        hsk_log_failure("ECIES inner decrypt failed");
        return -1;
    }

    if (inner_len != HANDSHAKE_INNER_PAYLOAD_SIZE) {
        fprintf(stderr, "HSK_FAIL:\n    Session key extraction failed\n");
        hsk_log_failure("inner payload size mismatch");
        return -1;
    }

    /* Extract session_id (little-endian) and session_key */
    uint32_t sid = (uint32_t)inner_plain[0]
                 | ((uint32_t)inner_plain[1] <<  8)
                 | ((uint32_t)inner_plain[2] << 16)
                 | ((uint32_t)inner_plain[3] << 24);

    memcpy(session_key_out, inner_plain + 4, SESSION_KEY_SIZE);

    /* Zero intermediate buffers to limit key exposure */
    memset(ct1, 0, sizeof(ct1));
    memset(inner_plain, 0, sizeof(inner_plain));

    if (session_id_out) *session_id_out = sid;

    hsk_log("HSK_SUCCESS", sid, "client extracted session key");
    return 0;
}

int handshake_client_build_subscribe(
    const priotps_identity_t *client_id,
    uint8_t *out_pkt, size_t *out_pkt_len)
{
    if (!client_id || !out_pkt || !out_pkt_len) return -1;

    seh_handshake_t seh;
    memset(&seh, 0, sizeof(seh));
    seh.flags = SEH_FLAG_HSK;
    fill_nonce(seh.nonce, sizeof(seh.nonce), 0x22);
    memcpy(seh.pub_key, client_id->public_key, 32);

    hsk_log("HSK_START", 0, "client sending SUBSCRIBE");
    return seh_hsk_encode(&seh, NULL, 0, out_pkt, out_pkt_len);
}

/**
 * @brief Build compact 5-byte ACK: [SEH_FLAG_ACK(1)][session_id(4 BE)]
 *
 * client_id is retained in the signature for API compatibility but is
 * not used for key material — the session_id uniquely identifies the
 * session on the server side.
 */
int handshake_client_build_ack(
    const priotps_identity_t *client_id,
    uint32_t session_id,
    uint8_t *out_pkt, size_t *out_pkt_len)
{
    if (!out_pkt || !out_pkt_len) return -1;
    (void)client_id; /* retained for API compat, unused in compact format */

    seh_ack_t ack;
    ack.flags      = SEH_FLAG_ACK;
    ack.session_id = session_id;

    hsk_log("HSK_ACK", session_id, "client sending compact ACK");
    return seh_ack_encode(&ack, out_pkt, out_pkt_len);
}

/* ------------------------------------------------------------------ */
/* Server-side Decomposed API (spec §Required Functions)              */
/* ------------------------------------------------------------------ */

/**
 * @brief Step 1 (server): parse SUBSCRIBE packet, extract client public key.
 */
int handshake_server_receive_subscribe(
    const uint8_t *in_pkt, size_t in_len,
    uint8_t client_pub_out[32])
{
    if (!in_pkt || !client_pub_out) return -1;
    if (in_len < SEH_HSK_OVERHEAD) return -1;

    seh_handshake_t hsk;
    size_t payload_off, payload_len;
    if (seh_hsk_decode(in_pkt, in_len, &hsk, &payload_off, &payload_len) != 0)
        return -1;

    if (hsk.flags != SEH_FLAG_HSK) return -1;

    memcpy(client_pub_out, hsk.pub_key, 32);
    hsk_log("HSK_START", 0, "server received SUBSCRIBE");
    return 0;
}

/**
 * @brief Step 2 (server): allocate pending session for connecting client.
 */
priotps_session_t *handshake_server_create_session(
    priotps_session_table_t *table,
    const uint8_t client_pub[32])
{
    if (!table || !client_pub) return NULL;

    priotps_session_t *s = session_create(table, client_pub);
    if (!s) {
        hsk_log_failure("session_create failed (table full)");
        return NULL;
    }
    /* state is already SESSION_PENDING from session_create() */
    hsk_log("SESSION_CREATED", s->session_id, "state=SESSION_PENDING");
    return s;
}

/**
 * @brief Step 7 (server): process incoming ACK.
 *
 * Accepts both wire formats:
 *   Compact (5 bytes): [flag][session_id(4 BE)] — spec format
 *   Legacy  (41+ bytes): [flag][nonce(8)][pubkey(32)] — backward compat
 */
int handshake_server_process_ack(
    priotps_session_table_t *table,
    const uint8_t *in_pkt, size_t in_len)
{
    if (!table || !in_pkt || in_len < 1) return -1;
    if ((in_pkt[0] & 0x03) != SEH_FLAG_ACK) return -1;

    priotps_session_t *s = NULL;

    int compact = seh_is_compact_ack(in_pkt, in_len);
    if (compact == 1) {
        /* --- Compact 5-byte ACK (spec format) --- */
        seh_ack_t ack;
        if (seh_ack_decode(in_pkt, in_len, &ack) != 0) {
            hsk_log_failure("seh_ack_decode failed");
            return -1;
        }
        s = session_find(table, ack.session_id);
        if (!s) {
            hsk_log_failure("compact ACK: session_id not found");
            return -1;
        }
    } else {
        /* --- Legacy 41-byte ACK (backward compat) --- */
        if (in_len < SEH_HSK_OVERHEAD) {
            hsk_log_failure("ACK too short for either format");
            return -1;
        }
        seh_handshake_t hsk;
        size_t payload_off, payload_len;
        if (seh_hsk_decode(in_pkt, in_len, &hsk, &payload_off, &payload_len) != 0) {
            hsk_log_failure("legacy ACK: seh_hsk_decode failed");
            return -1;
        }
        if (hsk.flags != SEH_FLAG_ACK) return -1;
        s = session_find_by_peer(table, hsk.pub_key);
        if (!s) {
            hsk_log_failure("legacy ACK: no matching session for peer pubkey");
            return -1;
        }
    }

    session_mark_established(s);
    hsk_log("HSK_ACK", s->session_id, NULL);
    hsk_log("SESSION_ESTABLISHED", s->session_id, "state=SESSION_ESTABLISHED");
    hsk_log("HSK_SUCCESS", s->session_id, NULL);
    return 0;
}

/**
 * @brief Spec decomposed API: process ACK by session_id directly.
 */
int handshake_server_process_ack_by_id(
    priotps_session_table_t *table,
    uint32_t session_id)
{
    if (!table || session_id == 0) return -1;

    priotps_session_t *s = session_find(table, session_id);
    if (!s) {
        hsk_log_failure("process_ack_by_id: session not found");
        return -1;
    }

    session_mark_established(s);
    hsk_log("HSK_ACK", session_id, NULL);
    hsk_log("SESSION_ESTABLISHED", session_id, "state=SESSION_ESTABLISHED");
    hsk_log("HSK_SUCCESS", session_id, NULL);
    return 0;
}
