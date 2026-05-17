#ifndef FRAGMENT_BUFFER_G7EES5IC

#define FRAGMENT_BUFFER_G7EES5IC

#include "messages.h"

/* XXX: Fragment buffer copies data from messages and does not store pointers to
 * messages. This is for clarity, one can optimize this later... */

enum MESSAGE_STATUS {
    MESSAGE_STATUS_READY, /* Message is ready to be delivered*/
    MESSAGE_STATUS_MISSING, /* Message is waiting for additional fragments */
    MESSAGE_STATUS_OLD,
    MESSAGE_STATUS_ERROR
};

enum FRAGMENT_STATUS {
    FRAGMENT_STATUS_MISSING,
    FRAGMENT_STATUS_OK,
    FRAGMENT_STATUS_DUP
};

struct fragment_buffer {
    bool clean;
    uint32_t seq_no;
    uint32_t frag_received;
    uint32_t frag_total;
    struct fragment_buffer_node* first_fragment;
    char* sid;
    struct fragment_buffer* next;
};

struct fragment_buffer_node {
    enum FRAGMENT_STATUS status;
    uint32_t frag_no;
    struct void_data data;
    struct fragment_buffer_node* next;
};

void create_fragment_buffer(char* sid, struct fragment_buffer** first);
void create_fragment_list(struct fragment_buffer* buffer, uint32_t size);
struct fragment_buffer* get_fragment_buffer(struct fragment_buffer* buf, char* sid);
enum FRAGMENT_STATUS add_fragment_to_buffer(struct fragment_buffer* buffer, struct void_data* data, uint32_t frag_no);
enum MESSAGE_STATUS check_buffer(struct fragment_buffer* buffer);
/* buffer               - fragment buffer
 * msg                  - received fragment
 * frag_next_missing    - fragment number of the next missing fragment, or 0 if messages is ready
 **/
enum MESSAGE_STATUS update_fragment_buffer(struct fragment_buffer* buffer, struct PRTP_packet* msg);
void free_fragment_list(struct fragment_buffer_node* first);
uint32_t frag_buf_data_length(struct fragment_buffer* buf);

/* These functions are for the server side fragmenting */
/* Fragments the data from a sensor data into fragment buffer */
int fragment_update_message(struct fragment_buffer* buf, struct PRTP_packet* msg, uint32_t max_data);
/* Fills in the data in the update message with the next fragment data */
int generate_update_messages(struct fragment_buffer* buf, struct PRTP_packet* msg);
void free_fragment_buffers(struct fragment_buffer* first);

#endif /* end of include guard: FRAGMENT_BUFFER_G7EES5IC */
