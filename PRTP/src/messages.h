
/**
 * @file messages.h
 * @brief PRIoTP message definitions and utilities.
 * 
 * Provides structures and functions for creating, manipulating, and
 * handling PRIoTP messages, including fragmentation and reassembly.
 * 
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 */


#ifndef MESSAGES_H
#define MESSAGES_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include "void_data.h"

#define BUFSIZE 1600

enum IOTMSG_TYPE  {
  LIST,
  LIST_RESPONSE,
  SUBSCRIBE,
  SUBSCRIBE_ACK,
  UPDATE,
  UPDATE_ACK,
  UPDATE_NACK,
  KEEP_ALIVE,
  UNSUBSCRIBE
};

/* Nodes are for lists in iotmsgs */
struct iotmsg_node {
  struct iotmsg_node* next;
  char* id;
};

struct iotmsg_list_node {
  struct iotmsg_node node;
};

struct iotmsg_subscribe_node {
  struct iotmsg_node node;
  bool reliable;
};

struct iotmsg_subscribe_ack_node {
  struct iotmsg_node node;
  uint32_t status;
};

/* Update message specific data */
struct update_data {
  char* sid;
  uint32_t sensor_type;
  struct void_data blob;
};

/* Base PRTP_packet structure with proper bitfield packing */
struct PRTP_packet {
  /* First 32-bit word - packed bitfields */
  uint32_t version:2;
  uint32_t type:4;
  uint32_t start_marker:1;
  uint32_t end_marker:1;
  uint32_t reliable:1;
  uint32_t fragmented:1;
  uint32_t utilize_timestamp:1;
  uint32_t seq_no:20;
  uint32_t _padding:1;  // Explicit padding to 32 bits
  
  /* Remaining fields are naturally aligned */
  uint32_t timestamp;
  uint32_t frag_no;
  uint32_t frag_total;
  
  struct void_data options;
  
  /* Union for different message type data */
  union {
    struct iotmsg_node* blob;      // For LIST_RESPONSE, SUBSCRIBE, SUBSCRIBE_ACK
    char* sid;                      // For UPDATE_ACK, UPDATE_NACK
    struct update_data update;      // For UPDATE messages
  } data;
};

struct PRTP_packet* create_iotmsg(enum IOTMSG_TYPE type);
void free_iotmsg(struct PRTP_packet*);
void iotmsg_copy_sids(struct PRTP_packet* dest, const struct iotmsg_node* src);
struct iotmsg_node* iotmsg_add_sid(struct PRTP_packet* msg, const char* id);

/* Message type specific setters for reliable field */
int iotmsg_set_sid_reliable(struct PRTP_packet* msg, bool reliable);
int iotmsg_update_set_reliable(struct PRTP_packet* msg, bool reliable);

/* Setters for update message fields */
int iotmsg_update_end_marker(struct PRTP_packet* msg, bool end_marker);
int iotmsg_update_fragmented(struct PRTP_packet* msg, bool fragmented);
int iotmsg_update_utilize_timestamp(struct PRTP_packet* msg, bool utilize_timestamp);

/* Used for update and update_ack message to set and get the sequence number and sid */
int iotmsg_set_seq(struct PRTP_packet* msg, uint32_t seq);
int iotmsg_set_sid(struct PRTP_packet* msg, const char* id);
int iotmsg_get_seq_no(const struct PRTP_packet* msg);
int iotmsg_set_timestamp(struct PRTP_packet* msg, uint32_t timestamp);

/* Used for update messages to set the fragmentation numbers */
int iotmsg_set_frag_no(struct PRTP_packet* msg, uint32_t no);
int iotmsg_set_frag_total(struct PRTP_packet* msg, uint32_t total);
int iotmsg_get_frag_no(const struct PRTP_packet* msg);
int iotmsg_get_frag_total(const struct PRTP_packet* msg);

/* Sensor type for UPDATE messages */
int iotmsg_set_sensor_type(struct PRTP_packet* msg, uint32_t type);

/* To set the status of subscription in subscribe ack */
int iotmsg_set_status(struct PRTP_packet* msg, int status);

/* To allocate buffer for data and set the whole buffer */
int iotmsg_alloc_data(struct PRTP_packet* msg);
int iotmsg_set_data(struct PRTP_packet* msg, const struct void_data* data);

/* To copy one value into already allocated data buffer */
int iotmsg_copy_data(struct PRTP_packet* msg, int offset, const struct void_data* value);
int send_iotmsg(int sd, const struct PRTP_packet* msg);

/* Helper functions for core PRTP protocol */
uint32_t generate_ntp_timestamp();
uint16_t generate_random_sequence_no();

void init_messages();
void shutdown_messages();

#endif