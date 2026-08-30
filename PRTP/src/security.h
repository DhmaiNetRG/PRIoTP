/**
 * @file security.h
 * @brief PRIoTPS Security — public API (PRIoTPS Phase 1, PSK-free).
 *
 * This module replaces the old PSK-based security layer.
 * Security is provided via:
 *   - BLAKE2s identity hashing
 *   - X25519 asymmetric key pairs
 *   - ECIES double-layer session key delivery
 *   - ASCON-AEAD128 payload encryption
 *
 * Integration points for transport.c:
 *   init_security_ctx()  — called from init_transport()
 *   shutdown_security_ctx() — called from shutdown_transport()
 *   get_priotps_security_ctx() — returns global ctx for encrypt/decrypt
 */

#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>
#include <stddef.h>
#include "security/security_core/security_core.h"
#include "security/seh/seh.h"

/* ============================================================
 * Return codes
 * ============================================================ */
#define PRIOTPS_OK               0
#define PRIOTPS_ERR_AUTH_FAIL   -1
#define PRIOTPS_ERR_TOO_SHORT   -2
#define PRIOTPS_ERR_NO_SESSION  -3
#define PRIOTPS_ERR_NULL_ARG    -4
#define PRIOTPS_ERR_HANDSHAKE   -5

/* ============================================================
 * Size constants (kept for transport.c compatibility)
 * ============================================================ */
#define PRIOTPS_MAX_OVERHEAD    (SEH_DATA_OVERHEAD + 16)  /* 13 + 16 = 29 bytes */
#define PRIOTPS_TAG_SIZE        16

/* Default identity file path */
#define PRIOTPS_DEFAULT_IDENTITY_FILE  "conf/priotps_identity.bin"

/**
 * @brief Initialize the global PRIoTPS security context.
 * Loads or generates node identity from identity_filepath.
 * @param identity_filepath Path to identity file (NULL = use default)
 */
void init_security_ctx(const char *identity_filepath);

/**
 * @brief Shut down the global PRIoTPS security context.
 * Zeroes all key material.
 */
void shutdown_security_ctx(void);

/**
 * @brief Return pointer to the global security context.
 * @return Non-NULL pointer; check ctx->initialized before use.
 */
priotps_security_ctx_t *get_priotps_security_ctx(void);

/**
 * @brief Returns human-readable error string for PRIOTPS_ERR_* codes.
 */
const char *priotps_strerror(int err);

#endif /* SECURITY_H */
