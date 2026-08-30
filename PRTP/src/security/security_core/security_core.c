#include "security_core.h"
#include "../identity/identity.h"
#include "../session/session.h"
#include "../handshake/handshake.h"
#include "../seh/seh.h"
#include "../../ascon.h"
#include "../../telemetry.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * @brief Initialize the security subsystem.
 */
int security_core_init(priotps_security_ctx_t *ctx, const char *identity_path) {
    if (!ctx || !identity_path) return -1;
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->identity_filepath, identity_path, sizeof(ctx->identity_filepath) - 1);

    if (identity_load_or_generate(&ctx->identity, identity_path) != 0) {
        return -1;
    }
    session_table_init(&ctx->sessions);
    ctx->initialized = 1;
    return 0;
}

/**
 * @brief Shut down the security subsystem.
 * Re-initializing zeroes the session table; identity is wiped manually.
 */
void security_core_shutdown(priotps_security_ctx_t *ctx) {
    if (!ctx) return;
    /* Zero session table by re-init (all slots marked inactive, keys zeroed) */
    session_table_init(&ctx->sessions);
    identity_wipe(&ctx->identity);
    ctx->initialized = 0;
}

/**
 * @brief Get the global node public key.
 */
void security_core_get_pub_key(const priotps_security_ctx_t *ctx, uint8_t pub_out[32]) {
    if (ctx && pub_out) {
        memcpy(pub_out, ctx->identity.public_key, 32);
    }
}

/**
 * @brief Encrypt a data payload using an active session key.
 */
int security_core_encrypt(priotps_security_ctx_t *ctx,
                            uint32_t session_id,
                            const uint8_t *plaintext, size_t pt_len,
                            uint8_t *out, size_t *out_len) {
    if (!ctx || !plaintext || !out || !out_len) return -1;

    priotps_session_t *session = session_find(&ctx->sessions, session_id);
    if (!session) return -1;

    uint8_t nonce8[8];
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (!urandom) return -1;
    if (fread(nonce8, 1, 8, urandom) != 8) {
        fclose(urandom);
        return -1;
    }
    fclose(urandom);

    uint8_t nonce16[16] = {0};
    memcpy(nonce16, nonce8, 8);

    /* ciphertext buffer: pt_len plaintext + 16 byte ASCON tag */
    uint8_t ciphertext[2048 + 16];
    if (pt_len + 16 > sizeof(ciphertext)) return -1;
    ascon_aead128_encrypt(ciphertext, plaintext, pt_len, NULL, 0, nonce16, session->session_key);

    seh_data_t seh;
    seh.flags      = SEH_FLAG_ENC;
    seh.session_id = session_id;
    memcpy(seh.nonce, nonce8, 8);

    size_t enc_len;
    if (seh_data_encode(&seh, ciphertext, pt_len + 16, out, &enc_len) != 0) {
        return -1;
    }

    *out_len = enc_len;
    return 0;
}

/**
 * @brief Decrypt an incoming wire packet.
 */
int security_core_decrypt(priotps_security_ctx_t *ctx,
                            const uint8_t *in, size_t in_len,
                            uint8_t *out, size_t *out_len,
                            uint32_t *session_id_out) {
    if (!ctx || !in || !out || !out_len || !session_id_out) return -1;

    seh_data_t seh;
    size_t ct_off, ct_len;

    if (seh_data_decode(in, in_len, &seh, &ct_off, &ct_len) != 0) {
        return -1;
    }

    priotps_session_t *session = session_find(&ctx->sessions, seh.session_id);
    if (!session) return -1;

    uint8_t nonce16[16] = {0};
    memcpy(nonce16, seh.nonce, 8);

    if (ascon_aead128_decrypt(out, in + ct_off, ct_len, NULL, 0, nonce16, session->session_key) != 0) {
        return -1;
    }

    *out_len   = ct_len - 16;
    *session_id_out = seh.session_id;
    session_touch(session);

    return 0;
}

/**
 * @brief Server: handle incoming handshake SUBSCRIBE packet.
 */
int security_core_server_handle_subscribe(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len,
    uint8_t *out_pkt, size_t *out_len,
    priotps_session_t **session_out) {

    if (!ctx || !in_pkt || !out_pkt || !out_len || !session_out) return -1;

    /* Parse incoming HSK SEH to get client public key */
    seh_handshake_t hsk;
    size_t sct_off, sct_len;
    if (seh_hsk_decode(in_pkt, in_len, &hsk, &sct_off, &sct_len) != 0) {
        return -1;
    }

    /* Build response: create session + double-ECIES encrypt session key */
    if (handshake_server_build_response(
            &ctx->identity,
            hsk.pub_key,
            &ctx->sessions,
            out_pkt, out_len,
            session_out) != 0) {
        return -1;
    }
    telemetry_session_created((*session_out)->session_id);

    return 0;
}

/**
 * @brief Client: process handshake response and extract session key.
 */
int security_core_client_process_handshake(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len,
    uint8_t session_key_out[16],
    uint8_t server_pub_out[32],
    uint32_t *session_id_out) {

    if (!ctx || !in_pkt || !session_key_out || !server_pub_out) return -1;

    if (handshake_client_process_response(
            &ctx->identity,
            in_pkt, in_len,
            session_key_out,
            server_pub_out,
            session_id_out) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Client: build SUBSCRIBE packet with embedded public key.
 */
int security_core_client_build_subscribe(
    priotps_security_ctx_t *ctx,
    uint8_t *out_pkt, size_t *out_len) {

    if (!ctx || !out_pkt || !out_len) return -1;
    return handshake_client_build_subscribe(&ctx->identity, out_pkt, out_len);
}

/**
 * @brief Client: build compact Handshake ACK packet.
 */
int security_core_client_build_ack(
    priotps_security_ctx_t *ctx,
    uint32_t session_id,
    uint8_t *out_pkt, size_t *out_len) {

    if (!ctx || !out_pkt || !out_len) return -1;
    return handshake_client_build_ack(&ctx->identity, session_id, out_pkt, out_len);
}

/**
 * @brief Add a client-side received session into the session table.
 *
 * If a session for this peer already exists, updates its session ID,
 * session key, and timestamp without allocating a new slot.
 * Otherwise allocates a new session slot.
 */
priotps_session_t *security_core_add_client_session(
    priotps_security_ctx_t *ctx,
    const uint8_t server_pub[32],
    const uint8_t session_key[16],
    uint32_t session_id) {

    if (!ctx || !server_pub || !session_key || session_id == 0) return NULL;

    /* Check if a session already exists for this peer to avoid leaking slots */
    priotps_session_t *s = session_find_by_peer(&ctx->sessions, server_pub);
    if (!s) {
        /* Allocate a new slot */
        s = session_create(&ctx->sessions, server_pub);
        if (!s) return NULL;
    }

    /* Set / update the server-assigned session ID and session key */
    s->session_id = session_id;
    memcpy(s->session_key, session_key, SESSION_KEY_SIZE);
    /* Store server pubkey for reference */
    memcpy(s->server_pubkey, server_pub, 32);
    s->last_seen  = (uint64_t)time(NULL);
    s->active     = 1;
    telemetry_session_created(session_id);

    return s;
}

/**
 * @brief Server: process incoming ACK → establish the matching session.
 *
 * Delegates to handshake_server_process_ack() which accepts both compact
 * (5-byte) and legacy (41-byte) ACK wire formats.
 * Emits telemetry_handshake_success() after successful establishment.
 */
int security_core_server_process_ack(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len)
{
    if (!ctx || !in_pkt || in_len == 0) return -1;
    if (!ctx->initialized) return -1;

    int ret = handshake_server_process_ack(&ctx->sessions, in_pkt, in_len);
    if (ret == 0) {
        /*
         * Emit telemetry for the newly-established session.
         * For compact ACK (5 bytes): session_id is at bytes [1..4] BE.
         * For legacy ACK (>=41 bytes): find by peer pubkey at offset 9.
         */
        uint32_t sid = 0;
        if (in_len == SEH_ACK_OVERHEAD) {
            /* Compact format */
            sid = ((uint32_t)in_pkt[1] << 24)
                | ((uint32_t)in_pkt[2] << 16)
                | ((uint32_t)in_pkt[3] <<  8)
                |  (uint32_t)in_pkt[4];
        } else if (in_len >= SEH_HSK_OVERHEAD) {
            /* Legacy format: pub_key at bytes [9..40] */
            const uint8_t *client_pub = in_pkt + 9;
            priotps_session_t *s = session_find_by_peer(&ctx->sessions, client_pub);
            if (s) sid = s->session_id;
        }
        if (sid) telemetry_handshake_success(sid, 0.0);
    }
    return ret;
}
