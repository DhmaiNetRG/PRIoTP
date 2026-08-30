#include "session.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    priotps_session_table_t table;
    session_table_init(&table);
    
    uint8_t peer_pub[32];
    memset(peer_pub, 0xAA, 32);
    
    priotps_session_t *s = session_create(&table, peer_pub);
    assert(s != NULL);
    assert(s->active == 1);
    
    priotps_session_t *s_find1 = session_find(&table, s->session_id);
    assert(s_find1 == s);
    
    priotps_session_t *s_find2 = session_find_by_peer(&table, peer_pub);
    assert(s_find2 == s);
    
    session_delete(s);
    assert(session_find(&table, s_find1->session_id) == NULL);
    
    // Test table full
    for (int i = 0; i < SESSION_MAX_ENTRIES; i++) {
        uint8_t pp[32] = { (uint8_t)i };
        assert(session_create(&table, pp) != NULL);
    }
    
    uint8_t extra[32] = { 0xFF };
    assert(session_create(&table, extra) == NULL);
    
    printf("Session tests passed.\n");
    return 0;
}
