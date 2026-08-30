#ifndef TELEMETRY_H
#define TELEMETRY_H
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the telemetry subsystem.
 * @param path Output file path (e.g. "priotps_telemetry.jsonl"). NULL = disabled.
 */
void telemetry_init(const char *path);

/** @brief Shutdown: flush and close the telemetry file. */
void telemetry_shutdown(void);

/* --- Handshake events --- */
void telemetry_handshake_start(uint32_t session_id, const char *peer_addr);
void telemetry_handshake_response(uint32_t session_id);
void telemetry_handshake_ack(uint32_t session_id);
void telemetry_handshake_success(uint32_t session_id, double latency_ms);
void telemetry_handshake_timeout(uint32_t session_id, int attempt);
void telemetry_handshake_retry(uint32_t session_id, int attempt);
void telemetry_handshake_failure(uint32_t session_id, const char *reason);

/* --- Session events --- */
void telemetry_session_created(uint32_t session_id);
void telemetry_session_expired(uint32_t session_id, double duration_s);

/* --- Data events --- */
void telemetry_encrypt(uint32_t session_id, size_t bytes);
void telemetry_decrypt(uint32_t session_id, size_t bytes);

/* --- Security events --- */
void telemetry_security_event(const char *type, uint32_t session_id, const char *detail);

#endif /* TELEMETRY_H */
