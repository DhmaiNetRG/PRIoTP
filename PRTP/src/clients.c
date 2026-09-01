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
#include "security.h"
#include "telemetry.h"

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
  struct client_node* node = NULL;
  struct transport aux_tr;
  int ret = -1;

  /* PRIoTPS Phase 4: Server-side Handshake Routing */
  char peek_buf[2048];
  ssize_t peek_len = recvfrom(sd, peek_buf, sizeof(peek_buf), MSG_PEEK, (struct sockaddr*)&from, &fromlen);

  if (peek_len > 0) {
      priotps_security_ctx_t *sec_ctx = get_priotps_security_ctx();
      if (sec_ctx && sec_ctx->initialized) {
          uint8_t flags = seh_get_flags((const uint8_t*)peek_buf, peek_len);

          if (flags == SEH_FLAG_HSK && peek_len < 109) {
              uint8_t out_pkt[2048];
              size_t out_len = 0;
              priotps_session_t *session_out = NULL;

              if (security_core_server_handle_subscribe(sec_ctx, (const uint8_t*)peek_buf, peek_len, out_pkt, &out_len, &session_out) == 0) {
                  /* Valid handshake request. Consume it. */
                  recvfrom(sd, peek_buf, sizeof(peek_buf), 0, (struct sockaddr*)&from, &fromlen);

                  /* Look up or add client */
                  for(node = clients_list; node != NULL; node = node->next) {
                      if(compare_sockaddr(&(node->transport.addr), &from) == 0) break;
                  }
                  if(node == NULL) {
                      log_print(l, "PRIoTPS: New client (handshake), adding to the list of clients.\n");
                      node = add_client(&from, fromlen, sd);
                  }

                  telemetry_handshake_start(session_out->session_id, "unknown");
                  ssize_t sent = sendto(sd, (const char*)out_pkt, out_len, 0, (struct sockaddr*)&from, fromlen);
                  if (sent > 0) {
                      node->handshake_done = 0; /* Wait for ACK */
                      node->session_id = session_out->session_id;
                      gettimeofday(&node->handshake_sent_at, NULL);
                      node->handshake_retries = 0;
                      memcpy(node->last_sct, out_pkt, out_len);
                      node->last_sct_len = out_len;
                      log_print(l, "PRIoTPS: Handshake response sent for session %u\n", session_out->session_id);
                      telemetry_handshake_response(session_out->session_id);
                  } else {
                      log_error(l, "PRIoTPS: Failed to send handshake response\n");
                  }

                  *msg = NULL;
                  *ret_node = node;
                  return (int)peek_len;
              }
              /* Handshake parsing failed (false positive). Do NOT consume. Fall through to transport_receive(). */
          }

          if (flags == SEH_FLAG_ACK) {
              /* Look up client FIRST using peek_from */
              for(node = clients_list; node != NULL; node = node->next) {
                  if(compare_sockaddr(&(node->transport.addr), &from) == 0) break;
              }
              if (node != NULL && node->handshake_done == 0) {
                  /* Valid ACK expected from this client — consume the packet */
                  recvfrom(sd, peek_buf, sizeof(peek_buf), 0, (struct sockaddr*)&from, &fromlen);
                  telemetry_handshake_ack(node->session_id);

                  /*
                   * Delegate to security_core_server_process_ack() which calls
                   * handshake_server_process_ack() → session_mark_established()
                   * transitioning state to SESSION_ESTABLISHED.
                   */
                  priotps_security_ctx_t *sctx = get_priotps_security_ctx();
                  if (sctx) {
                      security_core_server_process_ack(sctx, peek_buf, peek_len);
                  }

                  node->handshake_done = 1;
                  telemetry_handshake_success(node->session_id, 0.0);
                  log_print(l, "PRIoTPS: Handshake completed for session %u (SESSION_ESTABLISHED)\n", node->session_id);
                  *msg = NULL;
                  *ret_node = node;
                  return (int)peek_len;
              }
              /* Otherwise, it's either an ACK for a completed session or a legacy packet.
                 Fall through to transport_receive(). */
          }


          if (flags == SEH_FLAG_ENC) {
              for(node = clients_list; node != NULL; node = node->next) {
                  if(compare_sockaddr(&(node->transport.addr), &from) == 0) break;
              }
              if(!node || node->handshake_done != 1) {
                  /* Consume packet so it's dropped */
                  recvfrom(sd, peek_buf, sizeof(peek_buf), 0, (struct sockaddr*)&from, &fromlen);
                  log_error(l, "PRIoTPS: Encrypted data received before handshake completion. Dropped.\n");
                  *msg = NULL;
                  if (node) *ret_node = node;
                  return -1;
              }
              if (peek_len >= 13) {
                  uint32_t wire_sid = ((uint32_t)((uint8_t)peek_buf[9]) << 24) |
                                      ((uint32_t)((uint8_t)peek_buf[10]) << 16) |
                                      ((uint32_t)((uint8_t)peek_buf[11]) << 8) |
                                      ((uint32_t)((uint8_t)peek_buf[12]));
                  if (node->session_id != wire_sid) {
                      recvfrom(sd, peek_buf, sizeof(peek_buf), 0, (struct sockaddr*)&from, &fromlen);
                      log_error(l, "PRIoTPS: Cross-session spoofing attempt detected. Dropped.\n");
                      *msg = NULL;
                      *ret_node = node;
                      return -1;
                  }
              }
              /* Allow standard decrypt/BSON parsing by falling through to transport_receive() */
          }
      }
  }

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
  log_print(l, "Last seen at %ld.%06ld\n", (long)node->last_seen.tv_sec, (long)node->last_seen.tv_usec);
}

int check_handshake_timeouts(int sd)
{
  struct client_node* node;
  struct timeval now, tvelapsed;
  int timeouts = 0;

  gettimeofday(&now, 0);

  for (node = clients_list; node != NULL; node = node->next) {
    if (node->handshake_done == 0) {
      if (!timeval_subtract(&tvelapsed, &now, &node->handshake_sent_at)) {
        if (tvelapsed.tv_sec >= 5) {
          if (node->handshake_retries < 3) {
            node->handshake_retries++;
            sendto(sd, (const char*)node->last_sct, node->last_sct_len, 0, (struct sockaddr*)&node->transport.addr, node->transport.addr_len);
            telemetry_handshake_retry(node->session_id, node->handshake_retries);
            gettimeofday(&node->handshake_sent_at, NULL);
          } else {
            telemetry_handshake_failure(node->session_id, "max_retries");
            node->handshake_done = 255; /* Poison */
          }
          timeouts++;
        }
      }
    }
  }
  return timeouts;
}
