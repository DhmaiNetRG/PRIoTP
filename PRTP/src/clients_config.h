#ifndef CLIENTS_CONFIG_H
#define CLIENTS_CONFIG_H

#include "transport.h"
#include "clients.h"

void init_clients_config();
void shutdown_clients_config();

struct clients_config_node {
  struct clients_config_node* next;
  struct scheduler sched;
  struct client_node* client;
};

int read_clients_config(char* filename);

int get_scheduler(struct scheduler* sched, struct client_node* client);
int assign_scheduler(struct client_node* client);
int remove_client_config(struct client_node* client);

#endif
