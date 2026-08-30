/**
 * @file security.c
 * @brief PRIoTPS Security — global context management (PSK-free).
 *
 * Manages the single global priotps_security_ctx_t used by transport.c.
 * All actual cryptographic work is done in security/security_core/.
 */

#include "security.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>

static priotps_security_ctx_t g_security_ctx;
static struct logger *l = NULL;

void init_security_ctx(const char *identity_filepath)
{
    const char *path = identity_filepath ? identity_filepath
                                         : PRIOTPS_DEFAULT_IDENTITY_FILE;
    l = init_logger(stdout, stderr, stderr, "Security");
    memset(&g_security_ctx, 0, sizeof(g_security_ctx));

    if (security_core_init(&g_security_ctx, path) != 0) {
        log_error(l, "PRIoTPS: security_core_init failed (path=%s).\n", path);
        return;
    }
    log_print(l, "PRIoTPS: identity loaded from %s.\n", path);
    log_print(l, "PRIoTPS: security context initialized.\n");
}

void shutdown_security_ctx(void)
{
    security_core_shutdown(&g_security_ctx);
    if (l) {
        shutdown_logger(l);
        l = NULL;
    }
}

priotps_security_ctx_t *get_priotps_security_ctx(void)
{
    return &g_security_ctx;
}

const char *priotps_strerror(int err)
{
    switch (err) {
    case PRIOTPS_OK:             return "OK";
    case PRIOTPS_ERR_AUTH_FAIL:  return "authentication failure";
    case PRIOTPS_ERR_TOO_SHORT:  return "packet too short";
    case PRIOTPS_ERR_NO_SESSION: return "no active session";
    case PRIOTPS_ERR_NULL_ARG:   return "null argument";
    case PRIOTPS_ERR_HANDSHAKE:  return "handshake error";
    default:                      return "unknown error";
    }
}
