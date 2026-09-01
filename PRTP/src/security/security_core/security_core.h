#ifndef SECURITY_CORE_H
#define SECURITY_CORE_H
#include <stdint.h>
#include <stddef.h>
#include "../identity/identity.h"
#include "../session/session.h"

/** @brief Global security state for the node. */
typedef struct {
    priotps_identity_t  identity;   /* This node's asymmetric key pair */
    priotps_session_table_t sessions; /* Active session table */
    uint8_t  initialized;           /* 1 after security_core_init() */
    int      is_server;             /* 1 if server, 0 if client */
    char     identity_filepath[256]; /* Path to identity persistence file */
} priotps_security_ctx_t;

/**
 * @brief Initialize the security subsystem.
 * Loads or generates identity from 'identity_filepath'.
 * Initializes session table.
 * @param ctx           Security context to initialize
 * @param identity_path Path to identity file (e.g. "conf/priotps_identity.bin")
 * @return 0 on success, -1 on error
 */
int security_core_init(priotps_security_ctx_t *ctx, const char *identity_path);

/**
 * @brief Shut down the security subsystem.
 * Zeroes all key material before returning.
 * @param ctx Security context to shut down
 */
void security_core_shutdown(priotps_security_ctx_t *ctx);

/**
 * @brief Get the global node public key.
 * @param ctx Security context
 * @param pub_out Output: 32-byte public key
 */
void security_core_get_pub_key(const priotps_security_ctx_t *ctx,
                                 uint8_t pub_out[32]);

/**
 * @brief Encrypt a data payload using an active session key.
 *
 * Builds a complete wire packet: [Data SEH (13B)] [ciphertext] [tag(16B)]
 *
 * @param ctx        Security context
 * @param session_id Session to use for encryption
 * @param plaintext  Input plaintext
 * @param pt_len     Plaintext length
 * @param out        Output buffer (must be >= pt_len + SEH_DATA_OVERHEAD + 16)
 * @param out_len    Set to total output bytes
 * @return 0 on success, -1 on error (no session, etc.)
 */
int security_core_encrypt(priotps_security_ctx_t *ctx,
                            uint32_t session_id,
                            const uint8_t *plaintext, size_t pt_len,
                            uint8_t *out, size_t *out_len,
                            uint32_t seq_no, uint32_t timestamp, uint32_t frag_no);

/**
 * @brief Decrypt an incoming wire packet.
 *
 * Parses Data SEH, finds session, decrypts.
 *
 * @param ctx     Security context
 * @param in      Input wire buffer
 * @param in_len  Input length
 * @param out     Output plaintext buffer
 * @param out_len Set to plaintext bytes
 * @param session_id_out Set to the session_id used (for caller tracking)
 * @return 0 on success, -1 on error
 */
int security_core_decrypt(priotps_security_ctx_t *ctx,
                            const uint8_t *in, size_t in_len,
                            uint8_t *out, size_t *out_len,
                            uint32_t *session_id_out);

/**
 * @brief Server: handle incoming handshake SUBSCRIBE packet.
 *
 * Extracts client public key, creates session, builds SCT response.
 *
 * @param ctx          Security context
 * @param in_pkt       Incoming wire packet (HSK SEH from client)
 * @param in_len       Input length
 * @param out_pkt      Output response packet (SEH + SCT)
 * @param out_len      Set to output length
 * @param session_out  Set to created session pointer
 * @return 0 on success, -1 on error
 */
int security_core_server_handle_subscribe(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len,
    uint8_t *out_pkt, size_t *out_len,
    priotps_session_t **session_out);

/**
 * @brief Client: process handshake response and extract session key.
 *
 * Decrypts the server's double-ECIES SCT payload and extracts both the
 * 16-byte session key and the 32-bit server-assigned session_id.
 *
 * @param ctx            Security context  
 * @param in_pkt         Incoming server packet (SEH + SCT)
 * @param in_len         Input length
 * @param session_key_out Output 16-byte session key
 * @param server_pub_out  Output 32-byte server public key
 * @param session_id_out  Output server-assigned session ID
 *                        (may be NULL if caller does not need it)
 * @return 0 on success, -1 on error
 */
int security_core_client_process_handshake(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len,
    uint8_t session_key_out[16],
    uint8_t server_pub_out[32],
    uint32_t *session_id_out);

/**
 * @brief Client: build SUBSCRIBE packet with embedded public key.
 * @param ctx         Security context
 * @param out_pkt     Output buffer
 * @param out_len     Set to output bytes
 * @return 0 on success
 */
int security_core_client_build_subscribe(
    priotps_security_ctx_t *ctx,
    uint8_t *out_pkt, size_t *out_len);

/**
 * @brief Client: build compact Handshake ACK packet.
 * @param ctx        Security context
 * @param session_id Server-assigned session ID to embed in the ACK
 * @param out_pkt    Output buffer (>= SEH_ACK_OVERHEAD = 5 bytes)
 * @param out_len    Set to output bytes
 */
int security_core_client_build_ack(
    priotps_security_ctx_t *ctx,
    uint32_t session_id,
    uint8_t *out_pkt, size_t *out_len);

/**
 * @brief Add a received session (after client-side handshake).
 * Client calls this to store the session key it received from server.
 * @param ctx         Security context
 * @param server_pub  Server's 32-byte public key
 * @param session_key 16-byte session key
 * @param session_id  Session ID assigned by server
 * @return Pointer to created session entry, NULL on error
 */
priotps_session_t *security_core_add_client_session(
    priotps_security_ctx_t *ctx,
    const uint8_t server_pub[32],
    const uint8_t session_key[16],
    uint32_t session_id);

/**
 * @brief Server: process an incoming handshake ACK from the client.
 *
 * Parses the ACK packet, finds the matching PENDING session by the
 * client's embedded public key, and marks it SESSION_ESTABLISHED.
 *
 * @param ctx     Security context
 * @param in_pkt  Incoming ACK wire packet
 * @param in_len  Packet length
 * @return 0 on success (session established), -1 on error
 */
int security_core_server_process_ack(
    priotps_security_ctx_t *ctx,
    const uint8_t *in_pkt, size_t in_len);

#endif /* SECURITY_CORE_H */

