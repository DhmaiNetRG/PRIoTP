#ifndef SESSION_H
#define SESSION_H
#include <stdint.h>

#define SESSION_KEY_SIZE    16
#define SESSION_MAX_ENTRIES 64
#define SESSION_TIMEOUT_SEC 3600  /* 1 hour */

/**
 * @brief Session lifecycle state.
 */
typedef enum {
    SESSION_PENDING     = 0,  /* Created, awaiting client ACK */
    SESSION_ESTABLISHED = 1,  /* Handshake complete — ready for encrypted traffic */
    SESSION_EXPIRED     = 2,  /* Timed out or explicitly expired */
    SESSION_FAILED      = 3   /* Handshake or decryption failure */
} session_state_t;

/** @brief A single active session between server and client. */
typedef struct {
    uint32_t session_id;                     /* Unique session ID */
    uint8_t  session_key[SESSION_KEY_SIZE];  /* ASCON-AEAD128 symmetric key */
    uint8_t  peer_pub_key[32];               /* Peer's X25519 public key */
    uint8_t  server_pubkey[32];              /* Server's X25519 public key (set on client side) */
    session_state_t state;                   /* Lifecycle state */
    uint64_t created_at;                     /* Unix timestamp (seconds) of creation */
    uint64_t last_seen;                      /* Last activity Unix timestamp */
    uint8_t  active;                         /* 1 = slot occupied, 0 = free */
} priotps_session_t;

/** @brief Session table (stack-allocated, fixed max sessions). */
typedef struct {
    priotps_session_t entries[SESSION_MAX_ENTRIES];
    uint32_t next_id;  /* Auto-increment session ID counter */
} priotps_session_table_t;

/** @brief Initialize session table (zero all entries). */
void session_table_init(priotps_session_table_t *table);

/**
 * @brief Create a new session with a random session key.
 * New sessions start in SESSION_PENDING state.
 * @param table     Session table
 * @param peer_pub  Peer's 32-byte public key
 * @return Pointer to new session, or NULL if table is full
 */
priotps_session_t *session_create(priotps_session_table_t *table,
                                    const uint8_t peer_pub[32]);

/**
 * @brief Mark a session as fully established (handshake ACK received).
 * Transitions state from SESSION_PENDING → SESSION_ESTABLISHED.
 * @param session  Session pointer (internal use by handshake module)
 */
void session_mark_established(priotps_session_t *session);

/**
 * @brief Mark a session as established by table + session_id (spec API).
 * Looks up the session and calls session_mark_established().
 * @param table       Session table
 * @param session_id  ID of the session to establish
 */
void session_mark_established_by_id(priotps_session_table_t *table,
                                      uint32_t session_id);

/**
 * @brief Look up session by session_id.
 * @return Pointer to session, or NULL if not found or inactive
 */
priotps_session_t *session_find(priotps_session_table_t *table,
                                  uint32_t session_id);

/**
 * @brief Look up session by peer public key.
 * @return Pointer to session, or NULL if not found
 */
priotps_session_t *session_find_by_peer(priotps_session_table_t *table,
                                          const uint8_t peer_pub[32]);

/**
 * @brief Update last_seen timestamp for a session.
 */
void session_touch(priotps_session_t *session);

/**
 * @brief Delete a session (zero its memory and mark inactive).
 * Alias: session_remove() is provided for spec compatibility.
 */
void session_delete(priotps_session_t *session);

/**
 * @brief Remove a session by pointer (spec alias for session_delete).
 */
void session_remove(priotps_session_t *session);

/**
 * @brief Remove a session by table + session_id (spec API).
 * Looks up the session and calls session_delete().
 * @param table       Session table
 * @param session_id  ID of the session to remove
 */
void session_remove_by_id(priotps_session_table_t *table, uint32_t session_id);

/**
 * @brief Remove all expired sessions (last_seen + SESSION_TIMEOUT_SEC < now).
 * @return Number of sessions removed
 */
int session_expire(priotps_session_table_t *table);

#endif /* SESSION_H */
