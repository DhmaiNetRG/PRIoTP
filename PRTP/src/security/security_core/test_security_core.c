#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "security_core.h"

/**
 * @brief Stub test file for security_core module
 */
int main() {
    priotps_security_ctx_t ctx;
    assert(security_core_init(&ctx, "test_identity.bin") == 0);
    assert(ctx.initialized == 1);
    
    uint8_t pt[] = "Hello World";
    uint8_t ct[128];
    size_t ct_len = 0;
    
    uint8_t server_pub[32] = {1};
    uint8_t session_key[16] = {2};
    priotps_session_t *session = security_core_add_client_session(&ctx, server_pub, session_key, 1234);
    assert(session != NULL);
    
    assert(security_core_encrypt(&ctx, 1234, pt, sizeof(pt), ct, &ct_len) == 0);
    
    uint8_t out_pt[128];
    size_t out_pt_len = 0;
    uint32_t session_id_out = 0;
    
    assert(security_core_decrypt(&ctx, ct, ct_len, out_pt, &out_pt_len, &session_id_out) == 0);
    assert(session_id_out == 1234);
    assert(out_pt_len == sizeof(pt));
    assert(memcmp(pt, out_pt, out_pt_len) == 0);
    
    security_core_shutdown(&ctx);
    return 0;
}
