#include "clients.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "utils.h"
#include "bson_parser.h"
#include "bson_msg.h"
#include "subscriptions.h"
#include "logger.h"
#include "clients_config.h"

static struct client_node* clients_list = NULL;
static const struct timeval tv_keep_alive = {KEEP_ALIVE_TIMEOUT/1000, KEEP_ALIVE_TIMEOUT%1000};
static struct logger* l = NULL;
void clear_clients_list();

void init_clients()
{
  l = init_logger(stdout, stderr, stderr, "Clients");
}

void shutdown_clients()
{
  clear_clients_list();
  shutdown_logger(l);
}

void clear_clients_list()
{
  struct client_node* node = clients_list, *tmp = NULL;

  while( node != NULL ) {
    tmp = node->next;
    remove_client_subscriptions(node);
    free_transport(&(node->transport));
    free(node);
    node = tmp;
  }
}

/* Send a message to the client */
int send_client_message(struct transport_status* t_status, struct PRTP_packet* msg,
                        struct client_node* node)
{
  return transport_send(&(node->transport), t_status, msg);
}

struct client_node* add_client(const struct sockaddr_storage* addr, socklen_t len, int sd)
{
  struct client_node* node;

  node = xalloc(sizeof(struct client_node));

  memset(&node->transport.addr, 0, sizeof(struct sockaddr_storage));
  memcpy(&node->transport.addr, addr, len);
  node->transport.addr_len = len;
  node->transport.sd = sd;

  create_fragment_buffer( "none", &(node->transport.frag_buffer) );
  assign_scheduler(node);

  node->next = clients_list;
  gettimeofday(&node->last_seen, 0);
  clients_list = node;
  return node;
}

/* Read a packet from clients */
int read_client(int sd, struct client_node** ret_node, struct PRTP_packet** msg)
{
  struct sockaddr_storage from;
  socklen_t fromlen = sizeof(struct sockaddr);
  struct client_node* node;
  struct transport aux_tr;
  int ret = -1;

  aux_tr.sd = sd;
  ret = transport_receive(&aux_tr, msg, &from, &fromlen);
  if( (ret == -1) || (*msg == NULL) ) return -1;

  for( node = clients_list; node != NULL; node = node->next )
  {
    if( compare_sockaddr( &(node->transport.addr), &from ) == 0 ) break;
  }
  /* New client, add to the list */
  if( node == NULL ) {
    log_print(l, "New client, adding to the list of clients.\n");
    node = add_client(&from, fromlen, sd);
  }

  *ret_node = node;
  return ret;
}

int client_socket(const char* hostname, in_port_t port)
{
  int sd;

  if( (sd = init_socket(hostname, port, true, NULL)) == -1 ){
    log_error(l, "Client socket init failed.\n");
    return -1;
  }
  
  return sd;
}

int prune_expired_clients()
{
  struct client_node *node, *prev;
  struct timeval tv, tvelapsed, tvremain;
  int pruned = 0;

  gettimeofday(&tv, 0);
  /* log_debug(l, "Time is %d.%d\n", tv.tv_sec, tv.tv_usec); */
  node = clients_list;
  prev = NULL;
  while( node != NULL )
  {
    /* First if should be true or we've seen the client in future */
    /* log_debug(l, "Checking for timeout client.\n"); */
    /* print_client(node); */
    if( !timeval_subtract( &tvelapsed, &tv, &node->last_seen ) ) {
      /* if timeout - elapsed is negative, delete the client */
      if( timeval_subtract( &tvremain, &tv_keep_alive, &tvelapsed) ) {
        log_debug(l, "Client time out, deleting.\n");
        if( prev ) prev->next = node->next;
        else clients_list = node->next;
        /* Remove from subscriptions */
        remove_client_subscriptions(node);
        remove_client_config(node);
        free_transport(&(node->transport));
        free(node);
        pruned++;
        /* This is to free the node safely. List routines in utils would be nice to have */
        /* Especially cause there was a bug here when the head gets free'd */
        if( prev ) node = prev->next; else node = clients_list;
        if( node == NULL ) break; else continue;
      }
    }
    prev = node;
    node = node->next;
  }
  return pruned;
}

void print_client(struct client_node* node)
{
  log_print(l, "Last seen at %d.%d\n", node->last_seen.tv_sec, node->last_seen.tv_usec);
}
