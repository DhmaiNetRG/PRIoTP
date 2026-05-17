#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "messages.h"
#include "fragment_buffer.h"

void init_transport();
void shutdown_transport();
void init_congestion_control();
void shutdown_congestion_control();

struct transport_packet
{
  struct transport_packet* next;
  int seq_no;
  struct timeval when;
  int len;

  int frag_no;
  int frag_total;
};

/* Use same "polymorphism" idea as with messages */
struct data_packet 
{
  struct transport_packet descr;
  char* buf;
};

/* Congestion control schedulers */
struct scheduler
{
  int delay_msecs;
  float p_prob;
  float q_prob;

  int lossy_state;

  struct transport_packet* outgoing; /* The queue of packets scheduled for transmission */
};

struct transport_status
{
  struct transport_packet* sent; /* Packets sent by the peer */
  struct transport_packet* received; /* Packets received (no duplicates) */

  struct transport_packet* losses; /* Artificial lost packets */
  struct transport_packet* duplicates; /* Packets received (duplicates) */
  struct transport_packet* retransmitted; /* Packets sent (duplicates) */

  struct scheduler sched;
};

struct transport
{
  int sd; /* Socket descriptor */

  struct sockaddr_storage addr;
  int addr_len;

  bool skip_next;
  struct fragment_buffer* frag_buffer;
};

void free_transport_status(struct transport_status* status);
void free_transport(struct transport* transport);

/* Sliding window bitrate */
float bitrate(const struct transport_packet* pkts, struct timeval* since);

/* Sliding window packet count */
int pkt_count(const struct transport_packet* pkts, struct timeval* since);

/* When sending or receiving packet, update the transport status */
int transport_on_send(struct transport_status* status, struct transport_packet* pkt);
int transport_on_received(struct transport_status* status, struct transport_packet* pkt);
int transport_on_nack_received(struct transport_status* status, struct transport_packet* pkt);

/* Send a message */
int transport_send(struct transport* transport, struct transport_status* status,
                   struct PRTP_packet* msg);

/* Receive a message */
int transport_receive(struct transport* transport, struct PRTP_packet** msg,
                      struct sockaddr_storage* from, socklen_t* fromlen);

/* Used to pace outgoing packets */
int add_outgoing_packet(const struct transport* transport, struct transport_status* status,
                        struct transport_packet* pkt, const char* buf);

/* Is retransmission required? */
bool transport_retransmit(struct transport_status* status);

/* Flush the queues */
int transmit_from_queue(struct transport* transport, struct transport_status* status,
                        struct timeval* next_event);

#endif
