#ifndef CLIENTS_H
#define CLIENTS_H

#include <sys/time.h>
#include "transport.h"
#include "messages.h"

struct client_node {
  struct client_node* next;
  struct transport transport;
  struct timeval last_seen;
};

int client_socket(const char* hostname, in_port_t port);

/* Send response to the client */
int send_client_message(struct transport_status* t_status, struct PRTP_packet* msg, struct client_node* node);

int read_client(int sd, struct client_node** node, struct PRTP_packet** msg);

int prune_expired_clients();
void print_client(struct client_node* node);

void init_clients();
void shutdown_clients();

#endif
