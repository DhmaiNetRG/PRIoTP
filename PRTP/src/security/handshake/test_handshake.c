#include "handshake.h"
#include "../seh/seh.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    priotps_identity_t server_id, client_id;
    // Assuming identity_generate works
    identity_generate(&server_id);
    identity_generate(&client_id);
    
    priotps_session_table_t table;
    session_table_init(&table);
    
    uint8_t out_pkt[HANDSHAKE_MAX_PKT_SIZE];
    size_t out_pkt_len = sizeof(out_pkt);
    
    // 2. Client builds subscribe
    assert(handshake_client_build_subscribe(&client_id, out_pkt, &out_pkt_len) == 0);
    // decode
    seh_handshake_t seh;
    size_t p_off, p_len;
    assert(seh_hsk_decode(out_pkt, out_pkt_len, &seh, &p_off, &p_len) == 0);
    assert(memcmp(seh.pub_key, client_id.public_key, 32) == 0);
    
    // 3. Client builds ACK
    out_pkt_len = sizeof(out_pkt);
    assert(handshake_client_build_ack(&client_id, out_pkt, &out_pkt_len) == 0);
    assert(seh_hsk_decode(out_pkt, out_pkt_len, &seh, &p_off, &p_len) == 0);
    assert(seh.flags == SEH_FLAG_ACK);
    
    // 1. Full handshake simulation
    out_pkt_len = sizeof(out_pkt);
    priotps_session_t *session = NULL;
    assert(handshake_server_build_response(&server_id, client_id.public_key, &table, out_pkt, &out_pkt_len, &session) == 0);
    assert(session != NULL);
    
    uint8_t client_session_key[16];
    uint8_t server_pub_out[32];
    assert(handshake_client_process_response(&client_id, out_pkt, out_pkt_len, client_session_key, server_pub_out) == 0);
    assert(memcmp(client_session_key, session->session_key, 16) == 0);
    assert(memcmp(server_pub_out, server_id.public_key, 32) == 0);
    
    // 4. Wrong client private key fails decryption
    priotps_identity_t bad_client = client_id;
    bad_client.private_key[0] ^= 0x01; // flip a bit
    assert(handshake_client_process_response(&bad_client, out_pkt, out_pkt_len, client_session_key, server_pub_out) != 0);
    
    printf("Handshake tests passed.\n");
    return 0;
}
