#include "messages.h"
#include "utils.h"
#include "bson_msg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "sensor_types.h"
#include "security.h"   /* PRIoTPS Phase 4: encrypt control messages on egress */

static struct logger* l = NULL;

void init_messages()
{
  l = init_logger(stdout, stderr, stderr, "Messages");
}

void shutdown_messages()
{
  shutdown_logger(l);
}

int send_iotmsg(int sd, const struct PRTP_packet* msg) {
  char buf[BUFSIZE];
  size_t len = 0;
  const struct priotps_ctx *sec_ctx;

  /* Serialize PRTP_packet to BSON */
  len = serialize_iotmsg(msg, buf, BUFSIZE);

  /*
   * PRIoTPS Phase 4 — control-message encrypt path (send_iotmsg).
   *
   * send_iotmsg() is the fast path for control messages that do NOT
   * go through the congestion-control queue: LIST, SUBSCRIBE,
   * UPDATE_ACK, UPDATE_NACK, KEEP_ALIVE, UNSUBSCRIBE.
   *
   * After serialize_iotmsg() writes BSON into buf[0..len-1], we
   * overwrite the outgoing payload with the secured datagram:
   *
   *   wire payload = [12-byte SEH] [BSON ciphertext] [16-byte tag]
   *
   * secured_buf is sized for the maximum BSON payload (BUFSIZE) plus
   * the 28-byte PRIoTPS overhead (12 SEH + 16 tag).
   *
   * If security is disabled the original buf/len are sent unchanged.
   */
  sec_ctx = get_security_ctx();
  if (sec_ctx->enabled) {
    char secured_buf[BUFSIZE + PRIOTPS_MAX_OVERHEAD];
    size_t secured_len = 0;

    if (priotps_encrypt(sec_ctx, msg,
                        (const uint8_t *)buf, len,
                        (uint8_t *)secured_buf, &secured_len) == PRIOTPS_OK) {
      return send(sd, secured_buf, secured_len, 0);
    }

    log_error(l, "PRIoTPS: send_iotmsg encrypt failed for msg type %u;"
                 " dropping packet.\n", msg->type);
    return -1;
  }
  /* --- end PRIoTPS Phase 4 --- */

  /* Security disabled — send plaintext (existing behaviour) */
  return send(sd, buf, len, 0);
}

struct PRTP_packet* create_iotmsg(enum IOTMSG_TYPE type)
{
  struct PRTP_packet* result = NULL;

  switch(type) {
  case LIST:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case LIST_RESPONSE:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case SUBSCRIBE:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case SUBSCRIBE_ACK:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case UPDATE:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case UPDATE_ACK:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case UPDATE_NACK:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case KEEP_ALIVE:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  case UNSUBSCRIBE:
    result = xalloc(sizeof(struct PRTP_packet));
    break;
  }

  if( !result ) return NULL;

  result->type = type;

  return result;
}


// void free_iotsids(struct PRTP_packet* msg)
// {
//   struct iotmsg_node* sid, *next;

//   switch(msg->type) {
//   case LIST_RESPONSE:
//     for( sid = (msg)->data.blob; sid != NULL; sid = next ) {
//       next = sid->next;
//       free(sid->id);
//       free(sid);
//     }
//     break;
//   case SUBSCRIBE:
//     free( (msg)->data.blob );
//     break;
//   case SUBSCRIBE_ACK:
//     for( sid = (msg)->data.blob; sid != NULL; sid = next ) {
//       next = sid->next;
//       free(sid->id);
//       free(sid);
//     }
//     break;
//   case UPDATE:
//     free( (msg)->data.blob );
//     break;
//   case UPDATE_ACK:
//     free( (msg)->data.blob );
//     break;
//   case UPDATE_NACK:
//     free( (msg)->data.sid );
//     break;
//   }
// }


void free_iotsids(struct PRTP_packet* msg)
{
  struct iotmsg_node* sid, *next;

  switch(msg->type) {
  case LIST_RESPONSE:
  case SUBSCRIBE:
  case SUBSCRIBE_ACK:
    for( sid = msg->data.blob; sid != NULL; sid = next ) {
      next = sid->next;
      free(sid->id);
      free(sid);
    }
    break;
  case UPDATE:
    free(msg->data.update.sid);
    free(msg->data.update.blob.blob);
    break;
  case UPDATE_ACK:
  case UPDATE_NACK:
    free(msg->data.sid);
    break;
  default:
    break;
  }
}

/* Free PRTP_packet structure. */
void free_iotmsg(struct PRTP_packet* msg)
{
  if (msg == NULL) return;
  free_iotsids(msg);
  free(msg);
}

void iotmsg_copy_sids(struct PRTP_packet* dest, const struct iotmsg_node* src)
{
  const struct iotmsg_node* sid;

  for( sid = src; sid != NULL; sid = sid->next ) {
    iotmsg_add_sid(dest, sid->id);
  }
}

// int iotmsg_set_sid(struct PRTP_packet* msg, const char* id)
// {
//   int ret = -1;
//   struct PRTP_packet* upd_msg;
//   struct PRTP_packet* ack_msg;
//   struct PRTP_packet* nack_msg;

//   switch(msg->type) {
//   case UPDATE:
//     upd_msg = msg;
//     upd_msg->data.sid = xalloc(strlen(id) + 1);
//     strcpy(upd_msg->data.sid, id);
//     ret = 0;
//     break;

//   case UPDATE_ACK:
//     ack_msg = msg;
//     ack_msg->data.sid = xalloc(strlen(id) + 1);
//     strcpy(ack_msg->data.sid, id);
//     ret = 0;
//     break;

//   case UPDATE_NACK:
//     nack_msg = msg;
//     nack_msg->data.sid = xalloc(strlen(id) + 1);
//     strcpy(nack_msg->data.sid, id);
//     ret = 0;
//     break;
//   }
//   return ret;
// }

int iotmsg_set_sid(struct PRTP_packet* msg, const char* id)
{
  int ret = -1;
  size_t id_len = strlen(id) + 1;
  
  switch(msg->type) {
  case UPDATE:
    msg->data.update.sid = xalloc(id_len);
    strcpy(msg->data.update.sid, id);
    ret = 0;
    break;

  case UPDATE_ACK:
  case UPDATE_NACK:
    msg->data.sid = xalloc(id_len);
    strcpy(msg->data.sid, id);
    ret = 0;
    break;
  }
  return ret;
}



struct iotmsg_node* iotmsg_add_sid(struct PRTP_packet* msg, const char* id)
{
  struct iotmsg_node* node = NULL;
  struct PRTP_packet* sub_msg;
  struct PRTP_packet* sack_msg;
  struct PRTP_packet* list_msg;

  switch(msg->type) {
  case LIST_RESPONSE:
    list_msg =  msg;
    node = xalloc(sizeof(struct iotmsg_list_node));
    node->id = xalloc(strlen(id) + 1);
    strcpy(node->id, id);

    node->next = list_msg->data.blob;
    list_msg->data.blob = node;
    break;

  case SUBSCRIBE:
    sub_msg =  msg;
    node = xalloc(sizeof(struct iotmsg_subscribe_node));
    node->id = xalloc(strlen(id) + 1);
    strcpy(node->id, id);

    node->next = sub_msg->data.blob;
    sub_msg->data.blob = node;
    break;

  case SUBSCRIBE_ACK:
    sack_msg =  msg;
    node = xalloc(sizeof(struct iotmsg_subscribe_ack_node));
    node->id = xalloc(strlen(id) + 1);
    strcpy(node->id, id);

    node->next = sack_msg->data.blob;
    sack_msg->data.blob = node;
    break;
  }

  return node;
}

int iotmsg_set_sid_reliable(struct PRTP_packet* msg, bool reliable)
{
  struct iotmsg_subscribe_node* sid;

  if( msg->type != SUBSCRIBE )
    return -1;

  sid = (struct iotmsg_subscribe_node*)(msg)->data.blob;
  sid->reliable = (reliable ? 1 : 0);

  return 0;
}

int iotmsg_update_set_reliable(struct PRTP_packet* msg, bool reliable)
{
  msg->reliable =  (reliable ? 1 : 0);

  return 0;
}


int iotmsg_update_end_marker(struct PRTP_packet* msg, bool end_marker)
{

  msg->end_marker =(end_marker ? 1 : 0);

  return 0;
}

int iotmsg_update_fragmented(struct PRTP_packet* msg, bool fragmented)
{
  msg->fragmented = (fragmented ? 1 : 0);

  return 0;
}

int iotmsg_update_utilize_timestamp(struct PRTP_packet* msg, bool utilize_timestamp)
{

  msg->utilize_timestamp = (utilize_timestamp ? 1 : 0);

  return 0;
}

int iotmsg_set_timestamp(struct PRTP_packet* msg, uint32_t timestamp)
{
 msg->timestamp = timestamp;
 return 0;
}

int iotmsg_set_seq(struct PRTP_packet* msg, uint32_t seq)
{
   msg->seq_no = seq;
   return 0;
}



int iotmsg_get_seq_no(const struct PRTP_packet* msg)
{
  // int ret = -1;
  return msg->seq_no;
}

int iotmsg_set_frag_no(struct PRTP_packet* msg, uint32_t no) {
  int ret = -1;
  switch(msg->type) {
  case UPDATE:
    (msg)->frag_no = no;
    ret = 0;
    break;
  }
  return ret;
}

int iotmsg_get_frag_no(const struct PRTP_packet* msg) {
  int ret = -1;
  switch(msg->type) {
  case UPDATE:
    ret = (msg)->frag_no;
    break;
  }
  return ret;
}

int iotmsg_set_frag_total(struct PRTP_packet* msg, uint32_t total) {
  int ret = -1;
  switch(msg->type) {
  case UPDATE:
    (msg)->frag_total = total;
    ret = 0;
    break;
  }
  return ret;
}

// int iotmsg_set_sensor_type(struct PRTP_packet* msg, uint32_t type) {
//   int ret = -1;
//   switch(msg->type) {
//   case UPDATE:
//     msg->data.sensor_type = type;
//     ret = 0;
//     break;
//   }
//   return ret;
// }

int iotmsg_set_sensor_type(struct PRTP_packet* msg, uint32_t type) {
  int ret = -1;
  switch(msg->type) {
  case UPDATE:
    msg->data.update.sensor_type = type;
    ret = 0;
    break;
  }
  return ret;
}


int iotmsg_get_frag_total(const struct PRTP_packet* msg) {
  int ret = -1;
  switch(msg->type) {
  case UPDATE:
    ret = (msg)->frag_total;
    break;
  }
  return ret;
}

int iotmsg_set_status(struct PRTP_packet* msg, int status)
{
  if( msg->type != SUBSCRIBE_ACK )
    return -1;

  ((struct iotmsg_subscribe_ack_node*)(msg)->data.blob)->status = status;

  return 0;
}

// int iotmsg_alloc_data(struct PRTP_packet* msg)
// {
//   struct PRTP_packet* upd_msg =  msg;

//   if( msg->type != UPDATE )
//     return -1;

//   sensor_alloc_data(upd_msg->data.sensor_type, &(upd_msg->data));

//   return (upd_msg->data.blob == NULL)? -1 : 0;
// }
int iotmsg_alloc_data(struct PRTP_packet* msg)
{
  if( msg->type != UPDATE )
    return -1;

  sensor_alloc_data(msg->data.update.sensor_type, &(msg->data.update.blob));

  return (msg->data.update.blob.blob == NULL) ? -1 : 0;
}

// int iotmsg_set_data(struct PRTP_packet* msg, const struct void_data* data)
// {
//   struct PRTP_packet* upd_msg =  msg;

//   if( msg->type != UPDATE )
//     return -1;

//   log_debug( l, "iot message set data sending update. %s %d\n", upd_msg->data.sid ,  upd_msg->data.sensor_type );

//   return sensor_set_data(upd_msg->data.sensor_type, &(upd_msg->data), data);
// }

int iotmsg_set_data(struct PRTP_packet* msg, const struct void_data* data)
{
  if( msg->type != UPDATE )
    return -1;

  log_debug(l, "iot message set data sending update. %s %d\n", 
            msg->data.update.sid, msg->data.update.sensor_type);

  return sensor_set_data(msg->data.update.sensor_type, 
                        &(msg->data.update.blob), data);
}

/*int iotmsg_set_data_range(struct PRTP_packet* msg, const void* data, uint32_t offset, uint32_t length)
{
  struct PRTP_packet* upd_msg =  msg;
  int rval = -1;
  if ( msg->type != UPDATE)
    return -1;
  rval = sensor_set_data_range(upd_msg->sensor_type, &(upd_msg->data), data, offset, length);
  return rval;
}
*/

// int iotmsg_copy_data(struct PRTP_packet* msg, int offset, const struct void_data* value)
// {
//   int ret = -1;
//   struct PRTP_packet* upd_msg =  msg;

//   if( msg->type != UPDATE )
//     return ret;

//   ret = sensor_copy_offset_data(upd_msg->data.sensor_type, offset, &(upd_msg->data), value);

//   return ret;
// }

int iotmsg_copy_data(struct PRTP_packet* msg, int offset, const struct void_data* value)
{
  int ret = -1;

  if( msg->type != UPDATE )
    return ret;

  ret = sensor_copy_offset_data(msg->data.update.sensor_type, offset, 
                                &(msg->data.update.blob), value);

  return ret;
}

uint32_t generate_ntp_timestamp() {
    time_t current_time = time(NULL); // Get current time in seconds

    // NTP timestamp: 32 bits - 16 bits for seconds, 16 bits for fractions
    uint32_t ntp_seconds = (uint32_t)(current_time - 2208988800U); // Adjust for 1900 epoch
    uint32_t ntp_fraction = 0; // No nanosecond precision here

    return (ntp_seconds << 16) | (ntp_fraction >> 16); // Combine seconds and fraction
}

uint16_t generate_random_sequence_no() {
    return (uint16_t)(rand() & 0xFFFF);  // Mask to get a 16-bit value
}