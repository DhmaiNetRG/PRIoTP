#include "telemetry.h"
#include <stdio.h>
#include <time.h>

static FILE *g_tel_fp = NULL;

void telemetry_init(const char *path) {
    if (path == NULL) {
        g_tel_fp = NULL;
        return;
    }
    g_tel_fp = fopen(path, "a");
}

void telemetry_shutdown(void) {
    if (g_tel_fp) {
        fflush(g_tel_fp);
        fclose(g_tel_fp);
        g_tel_fp = NULL;
    }
}

void telemetry_handshake_start(uint32_t session_id, const char *peer_addr) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeStart\",\"session_id\":%u,\"peer\":\"%s\"}\n",
            (double)time(NULL), session_id, peer_addr);
    fflush(g_tel_fp);
}

void telemetry_handshake_response(uint32_t session_id) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeResponse\",\"session_id\":%u}\n",
            (double)time(NULL), session_id);
    fflush(g_tel_fp);
}

void telemetry_handshake_ack(uint32_t session_id) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeAck\",\"session_id\":%u}\n",
            (double)time(NULL), session_id);
    fflush(g_tel_fp);
}

void telemetry_handshake_success(uint32_t session_id, double latency_ms) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeSuccess\",\"session_id\":%u,\"latency_ms\":%.2f}\n",
            (double)time(NULL), session_id, latency_ms);
    fflush(g_tel_fp);
}

void telemetry_handshake_timeout(uint32_t session_id, int attempt) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeTimeout\",\"session_id\":%u,\"attempt\":%d}\n",
            (double)time(NULL), session_id, attempt);
    fflush(g_tel_fp);
}

void telemetry_handshake_retry(uint32_t session_id, int attempt) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeRetry\",\"session_id\":%u,\"attempt\":%d}\n",
            (double)time(NULL), session_id, attempt);
    fflush(g_tel_fp);
}

void telemetry_handshake_failure(uint32_t session_id, const char *reason) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"HandshakeFailure\",\"session_id\":%u,\"reason\":\"%s\"}\n",
            (double)time(NULL), session_id, reason);
    fflush(g_tel_fp);
}

void telemetry_session_created(uint32_t session_id) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"SessionCreated\",\"session_id\":%u}\n",
            (double)time(NULL), session_id);
    fflush(g_tel_fp);
}

void telemetry_session_expired(uint32_t session_id, double duration_s) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"SessionExpired\",\"session_id\":%u,\"duration_s\":%.1f}\n",
            (double)time(NULL), session_id, duration_s);
    fflush(g_tel_fp);
}

void telemetry_encrypt(uint32_t session_id, size_t bytes) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"Encrypt\",\"session_id\":%u,\"bytes\":%zu}\n",
            (double)time(NULL), session_id, bytes);
    fflush(g_tel_fp);
}

void telemetry_decrypt(uint32_t session_id, size_t bytes) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"Decrypt\",\"session_id\":%u,\"bytes\":%zu}\n",
            (double)time(NULL), session_id, bytes);
    fflush(g_tel_fp);
}

void telemetry_security_event(const char *type, uint32_t session_id, const char *detail) {
    if (!g_tel_fp) return;
    fprintf(g_tel_fp, "{\"ts\":%.3f,\"event\":\"SecurityEvent\",\"type\":\"%s\",\"session_id\":%u,\"detail\":\"%s\"}\n",
            (double)time(NULL), type, session_id, detail);
    fflush(g_tel_fp);
}
