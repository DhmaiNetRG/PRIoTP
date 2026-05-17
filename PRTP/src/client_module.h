#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <sys/time.h>
#include "messages.h"
#include "logger.h"

#define BUFSIZE 1600

struct pending_sub_request
{
  struct pending_sub_request* next;
  struct PRTP_packet* submsg;
  struct timeval time_sent;
};

void init_client_module();
void shutdown_client_module();

int send_iotmsg(int sd, const struct PRTP_packet* msg);
int send_keep_alive(int sd);
int send_subscribe_all(int sd, struct iotmsg_node* sids, bool reliable);
int send_unsubscribe(int sd);
int send_update_ack(int sd, struct PRTP_packet* upd_msg);

int query_sensor_list(int sd);
void print_list_response(struct PRTP_packet* msg);

void free_pending_sub_request(struct pending_sub_request * subreq);
void remove_pending_sub_request(struct iotmsg_node * node);
void add_pending_subscription(struct PRTP_packet * msg);
void update_pending_subscriptions(int sd);

void add_fragment_buffer( char* sid );
struct fragment_buffer* get_frag_buffer( char* sid );

void add_active_flow( char* sid );
struct active_flow* get_active_flow( char* sid );
void check_active_flow(struct PRTP_packet* upd_msg, int sd);

#endif
