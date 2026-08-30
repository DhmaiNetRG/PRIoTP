#ifndef HANDSHAKE_H
#define HANDSHAKE_H
#include <stdint.h>
#include <stddef.h>
#include "../identity/identity.h"
#include "../session/session.h"

#define HANDSHAKE_INNER_PAYLOAD_SIZE  20   /* 4-byte session_id (LE) + 16-byte session_key */
#define HANDSHAKE_MAX_SCT_SIZE  512  /* max bytes for double-ECIES ciphertext */
#define HANDSHAKE_MAX_PKT_SIZE  600  /* max wire packet for handshake */

/**
 * @brief Server: build a handshake response packet (SEH + double-encrypted session key).
 *
 * Called after server receives a client SUBSCRIBE with HSK SEH.
 * Creates session, double-encrypts session key, returns wire packet.
 *
 * @param server_id     Server's identity (private + public key)
 * @param client_pub    Client's 32-byte public key (from incoming HSK SEH)
 * @param table         Server session table
 * @param out_pkt       Output wire buffer for the SEH+SCT packet
 * @param out_pkt_len   Set to total output bytes
 * @param session_out   Set to pointer of created session (for session_id tracking)
 * @return 0 on success, -1 on error
 */
int handshake_server_build_response(
    const priotps_identity_t *server_id,
    const uint8_t client_pub[32],
    priotps_session_table_t *table,
    uint8_t *out_pkt, size_t *out_pkt_len,
    priotps_session_t **session_out);

/**
 * @brief Client: process incoming handshake SEH packet and extract session key.
 *
 * Called when client receives a packet with SEH_FLAG_HSK from server.
 * Decrypts the double-ECIES SCT payload.  The inner plaintext has the
 * layout:
 *
 *   [session_id : 4 bytes LE] [session_key : 16 bytes]
 *
 * @param client_id       Client's identity
 * @param pkt             Incoming wire buffer (full SEH+SCT packet)
 * @param pkt_len         Length of incoming buffer
 * @param session_key_out Output: 16-byte session key
 * @param server_pub_out  Output: 32-byte server public key (from SEH)
 * @param session_id_out  Output: server-assigned 32-bit session ID
 *                        (may be NULL if caller does not need it)
 * @return 0 on success, -1 on error
 */
int handshake_client_process_response(
    const priotps_identity_t *client_id,
    const uint8_t *pkt, size_t pkt_len,
    uint8_t session_key_out[16],
    uint8_t server_pub_out[32],
    uint32_t *session_id_out);

/**
 * @brief Client: build a handshake SUBSCRIBE packet with public key embedded.
 *
 * @param client_id    Client's identity
 * @param out_pkt      Output wire buffer
 * @param out_pkt_len  Set to output bytes
 * @return 0 on success, -1 on error
 */
int handshake_client_build_subscribe(
    const priotps_identity_t *client_id,
    uint8_t *out_pkt, size_t *out_pkt_len);

/**
 * @brief Build a Handshake ACK packet (client → server).
 *
 * Generates the spec-compliant compact ACK:
 *   [SEH_FLAG_ACK (1 byte)] [session_id (4 bytes BE)]
 *
 * @param client_id    Client's identity
 * @param session_id   Session ID assigned by the server
 * @param out_pkt      Output wire buffer (must be >= SEH_ACK_OVERHEAD)
 * @param out_pkt_len  Set to output bytes (SEH_ACK_OVERHEAD = 5)
 * @return 0 on success, -1 on error
 */
int handshake_client_build_ack(
    const priotps_identity_t *client_id,
    uint32_t session_id,
    uint8_t *out_pkt, size_t *out_pkt_len);

/* ------------------------------------------------------------------ */
/* Server-side decomposed API (spec §Required Functions)               */
/* ------------------------------------------------------------------ */

/**
 * @brief Server: parse an incoming SUBSCRIBE (HSK) packet and extract
 *        the client public key.
 *
 * @param in_pkt        Incoming wire buffer
 * @param in_len        Buffer length
 * @param client_pub_out Output: 32-byte client public key
 * @return 0 on success, -1 if packet is malformed or flag mismatch
 */
int handshake_server_receive_subscribe(
    const uint8_t *in_pkt, size_t in_len,
    uint8_t client_pub_out[32]);

/**
 * @brief Server: allocate a new pending session for the connecting client.
 *
 * Wraps session_create(). New session state = SESSION_PENDING.
 *
 * @param table       Server session table
 * @param client_pub  Client's 32-byte public key
 * @return Pointer to new session entry, NULL on table-full
 */
priotps_session_t *handshake_server_create_session(
    priotps_session_table_t *table,
    const uint8_t client_pub[32]);

/**
 * @brief Server: process an incoming ACK packet — accepts both wire formats.
 *
 * Compact ACK (5 bytes): extracts session_id and marks session established.
 * Legacy ACK (41 bytes): extracts peer public key, looks up session by peer.
 * Both formats result in session_mark_established().
 *
 * @param table     Server session table
 * @param in_pkt    Incoming wire buffer (SEH_FLAG_ACK packet)
 * @param in_len    Buffer length
 * @return 0 on success (session found and established), -1 on error
 */
int handshake_server_process_ack(
    priotps_session_table_t *table,
    const uint8_t *in_pkt, size_t in_len);

/**
 * @brief Server: process ACK by session_id directly (spec decomposed API).
 *
 * @param table      Server session table
 * @param session_id Session to mark established
 * @return 0 on success, -1 if session not found
 */
int handshake_server_process_ack_by_id(
    priotps_session_table_t *table,
    uint32_t session_id);

#endif /* HANDSHAKE_H */


