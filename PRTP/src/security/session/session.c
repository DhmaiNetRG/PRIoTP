#include "session.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

void session_table_init(priotps_session_table_t *table) {
    if (!table) return;
    memset(table->entries, 0, sizeof(table->entries));
    table->next_id = 1;
}

priotps_session_t *session_create(priotps_session_table_t *table, const uint8_t peer_pub[32]) {
    if (!table) return NULL;
    
    priotps_session_t *slot = NULL;
    for (int i = 0; i < SESSION_MAX_ENTRIES; i++) {
        if (!table->entries[i].active) {
            slot = &table->entries[i];
            break;
        }
    }
    if (!slot) return NULL;
    
    slot->session_id = table->next_id++;
    if (table->next_id == 0) table->next_id = 1; // skip 0
    
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        fread(slot->session_key, 1, SESSION_KEY_SIZE, f);
        fclose(f);
    } else {
        memset(slot->session_key, 0x55, SESSION_KEY_SIZE);
    }
    
    slot->created_at = (uint64_t)time(NULL);
    slot->last_seen = slot->created_at;
    memcpy(slot->peer_pub_key, peer_pub, 32);
    memset(slot->server_pubkey, 0, 32);
    slot->state  = SESSION_PENDING;   /* awaiting ACK */
    slot->active = 1;
    
    return slot;
}

void session_mark_established(priotps_session_t *session) {
    if (session && session->active) {
        session->state    = SESSION_ESTABLISHED;
        session->last_seen = (uint64_t)time(NULL);
    }
}



priotps_session_t *session_find(priotps_session_table_t *table, uint32_t session_id) {
    if (!table) return NULL;
    for (int i = 0; i < SESSION_MAX_ENTRIES; i++) {
        if (table->entries[i].active && table->entries[i].session_id == session_id) {
            fprintf(stderr, "AUDIT: SESSION_LOOKUP sid=%u found=%p\n", session_id, (void*)&table->entries[i]);
            return &table->entries[i];
        }
    }
    fprintf(stderr, "AUDIT: SESSION_LOOKUP sid=%u found=(nil)\n", session_id);
    return NULL;
}

priotps_session_t *session_find_by_peer(priotps_session_table_t *table, const uint8_t peer_pub[32]) {
    if (!table) return NULL;
    for (int i = 0; i < SESSION_MAX_ENTRIES; i++) {
        if (table->entries[i].active && memcmp(table->entries[i].peer_pub_key, peer_pub, 32) == 0) {
            return &table->entries[i];
        }
    }
    return NULL;
}

void session_touch(priotps_session_t *session) {
    if (session) {
        session->last_seen = (uint64_t)time(NULL);
    }
}

void session_delete(priotps_session_t *session) {
    if (session) {
        memset(session, 0, sizeof(priotps_session_t));
    }
}

/**
 * @brief Remove a session — spec-compatible alias for session_delete().
 */
void session_remove(priotps_session_t *session) {
    session_delete(session);
}

/**
 * @brief Spec API: mark session established by table + session_id.
 */
void session_mark_established_by_id(priotps_session_table_t *table,
                                      uint32_t session_id) {
    priotps_session_t *s = session_find(table, session_id);
    session_mark_established(s);
}

/**
 * @brief Spec API: remove session by table + session_id.
 */
void session_remove_by_id(priotps_session_table_t *table, uint32_t session_id) {
    priotps_session_t *s = session_find(table, session_id);
    session_delete(s);
}

int session_expire(priotps_session_table_t *table) {
    if (!table) return 0;
    int count = 0;
    uint64_t now = (uint64_t)time(NULL);
    
    for (int i = 0; i < SESSION_MAX_ENTRIES; i++) {
        if (table->entries[i].active) {
            if (now - table->entries[i].last_seen > SESSION_TIMEOUT_SEC) {
                table->entries[i].state = SESSION_EXPIRED;
                session_delete(&table->entries[i]);
                count++;
            }
        }
    }
    return count;
}

