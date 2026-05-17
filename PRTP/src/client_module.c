#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#include "utils.h"
#include "logger.h"
#include "active_flow.h"
#include "client_module.h"
#include "bson_msg.h"
#include "messages.h"
#include "subscriptions.h"
#include "fragment_buffer.h"

static struct logger* l = NULL;
static struct pending_sub_request * pending_subs;
static struct fragment_buffer * fragment_buffer_list = NULL;
static struct active_flow * active_flow_list = NULL;

void init_client_module()
{
  l = init_logger(stdout, stderr, stderr, "Client Module");
  init_active_flow();
}


int query_sensor_list(int sd) {
  size_t len = 0;
  struct PRTP_packet* msg = create_iotmsg(LIST);
  len = send_iotmsg(sd, msg);
  free_iotmsg(msg);
  return len;
}

void print_list_response(struct PRTP_packet* msg)
{
  struct iotmsg_node* node;
  log_print(l, "Sensors: \n");
  if( msg->data.blob == NULL ) log_print(l, "no sensors\n");
  for( node = msg->data.blob; node != NULL; node = node->next ) {
    log_print( l, "%s\n", node->id );
  }
}

void free_pending_sub_request(struct pending_sub_request * subreq) 
{
  free_iotmsg((struct PRTP_packet *)subreq->submsg);
  free(subreq);
}

void clear_pending_subs()
{
  struct pending_sub_request* node = pending_subs;

  while( node != NULL ) {
    pending_subs = node->next;
    free_pending_sub_request(node);
    node = pending_subs;
  }
}

void remove_pending_sub_request(struct iotmsg_node * node)
{
  struct pending_sub_request * pending;
  struct pending_sub_request * prev_pending = NULL;
  struct iotmsg_node * pnode_pending = NULL;
  struct iotmsg_node * pnode = NULL;

  assert(node != NULL);
  assert(node->id != NULL);

  for (pending = pending_subs; pending != NULL; pending = pending->next) {
    bool msg_match = true;
    for (pnode_pending = pending->submsg->data.blob; pnode_pending != NULL; pnode_pending = pnode_pending->next) {
      bool node_match = false;
      for (pnode = node; pnode != NULL; pnode = pnode->next) {
        if (strcmp(pnode_pending->id, pnode->id) == 0) {
          node_match = true;
          break;
        }
      }
      if (!node_match) {
        msg_match = false;
        break;
      }
    }
    if (msg_match) {		
      if (prev_pending != NULL) {
        prev_pending->next = pending->next;
      } else {
        pending_subs = pending->next;
      }
      free_pending_sub_request(pending);
      return;
    } else {
      prev_pending = pending;
    }
  }
  log_print( l, "requested to remove a subscription but no such subscription found\n" );
}

void add_pending_subscription(struct PRTP_packet * msg) {
  struct pending_sub_request* subreq = (struct pending_sub_request *)xalloc( sizeof(struct pending_sub_request) );
  subreq->submsg = msg;
  gettimeofday( &subreq->time_sent, NULL);
  subreq->next = pending_subs;
  pending_subs = subreq;
}

void update_pending_subscriptions(int sd) {
  struct pending_sub_request * sub;
  struct timeval time_now;
  gettimeofday(&time_now, NULL);
  for (sub = pending_subs; sub != NULL; sub = sub->next) {
    if( ( time_now.tv_sec * 1000 + time_now.tv_usec / 1000 ) -
        ( sub->time_sent.tv_sec * 1000 + sub->time_sent.tv_usec / 1000 ) > RETRANSMIT_TIMEOUT) {
      send_iotmsg(sd, sub->submsg);
      sub->time_sent = time_now;
    }
  }
}

int send_keep_alive(int sd) {
  size_t len = 0;
  struct PRTP_packet* msg = create_iotmsg(KEEP_ALIVE);
  len = send_iotmsg(sd, msg);
  free_iotmsg(msg);
  return len;
}

int send_subscribe_all(int sd, struct iotmsg_node* sids, bool reliable)
{
  size_t len = 0;
  struct PRTP_packet* msg = create_iotmsg(SUBSCRIBE);
  struct iotmsg_node* node;
  struct PRTP_packet* sub_msg = msg;

  iotmsg_copy_sids( msg, sids );
  for( node = sub_msg->data.blob; node != NULL; node = node->next ) {
    /* Let all subscriptions be configured when subscribing to all sensors*/
    ((struct iotmsg_subscribe_node*)node)->reliable = reliable;
  }
  len = send_iotmsg(sd, msg);
  free_iotmsg(msg);
  return len;
}

int send_unsubscribe(int sd){
  size_t len = 0;
  struct PRTP_packet* msg = create_iotmsg(UNSUBSCRIBE);
  len = send_iotmsg(sd, msg);
  free_iotmsg(msg);
  return len;
}

int send_update_ack(int sd, struct PRTP_packet* upd_msg)
{
  size_t len = 0;
  struct PRTP_packet* msg = create_iotmsg(UPDATE_ACK);
  struct PRTP_packet* ack_msg = msg;

  ack_msg->seq_no = upd_msg->seq_no;
  iotmsg_set_sid( msg, upd_msg->data.sid );

  log_debug( l, "---------Sending Update acknowledgment from client.-----------\n");

  len = send_iotmsg(sd, msg);
  free_iotmsg(msg);
  return len;
}

void add_fragment_buffer( char* sid )
{
  create_fragment_buffer(sid, &fragment_buffer_list);
}

struct fragment_buffer* get_frag_buffer( char* sid )
{
  if( sid == NULL ) return fragment_buffer_list;
  else return get_fragment_buffer( fragment_buffer_list, sid );
}

void add_active_subscription( char* sid )
{
  create_active_flow(sid, &active_flow_list);
}

void add_active_flow( char* sid )
{
  create_active_flow(sid, &active_flow_list);
}

struct active_flow* get_active_flow( char* sid )
{
  if( sid == NULL ) return active_flow_list;
  else return get_active_flow_from_list( active_flow_list, sid );
}

void check_active_flow( struct PRTP_packet* upd_msg, int sd)
{
  enum FLOW_STATUS_REPORT report;
  struct active_flow* target_flow = get_active_flow( upd_msg->data.sid );
  if ( target_flow == NULL ) {
    log_print(l, "tried to access ");
    return;
  }
  report = update_active_flow(target_flow, upd_msg->seq_no);

  if ( report == FLOW_STATUS_REPORT_FRESH_MISS ) {
    struct PRTP_packet* nack_msg = create_iotmsg(UPDATE_NACK);
    iotmsg_set_seq(nack_msg, iotmsg_get_seq_no( (const struct PRTP_packet*)upd_msg ) );
    iotmsg_set_sid(nack_msg, upd_msg->data.sid);
    send_iotmsg(sd, nack_msg);
    free_iotmsg(nack_msg);
  }
}

void shutdown_client_module()
{
  shutdown_logger(l);
  clear_pending_subs();
  free_fragment_buffers(fragment_buffer_list);
}

