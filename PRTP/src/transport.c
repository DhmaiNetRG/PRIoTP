#include "transport.h"
#include "utils.h"
#include "messages.h"
#include "bson_msg.h"
#include "client_module.h"
#include "logger.h"
#include "bson_parser.h"

#include <stdlib.h>
#include <time.h>

#undef BUF_SIZE
#define BUF_SIZE 1600

static const struct timeval tv_retransmit = {RETRANSMIT_TIMEOUT/1000, (1000*RETRANSMIT_TIMEOUT)};

static struct logger* l = NULL;

void init_transport()
{
  srand (time(NULL));
  l = init_logger(stdout, stderr, stderr, "Transport");
  init_congestion_control();
}

void shutdown_transport()
{
  shutdown_logger(l);
  shutdown_congestion_control();
}

struct transport_packet* new_pkt(struct transport_packet** to, int seq_no,
                                 const struct timeval* when, int len, int frag_no, int frag_total)
{
  struct transport_packet* node;
  node = xalloc(sizeof(struct transport_packet));

  node->seq_no = seq_no;
  node->when = *when;
  node->len = len;

  node->frag_no = frag_no;
  node->frag_total = frag_total;

  node->next = *to;
  *to = node;

  return node;
}

struct transport_packet* add_pkt(struct transport_packet** to, struct transport_packet* pkt)
{
  struct transport_packet* node;
  node = xalloc(sizeof(struct transport_packet));

  node->seq_no = pkt->seq_no;
  node->when = pkt->when;
  node->len = pkt->len;

  node->frag_no = pkt->frag_no;
  node->frag_total = pkt->frag_total;

  node->next = *to;
  *to = node;

  return node;
}

void free_pkts(struct transport_packet* head)
{
  struct transport_packet* node = head, *tmp = NULL;

  while( node != NULL ) {
    tmp = node->next;
    free(node);
    node = tmp;
  }
}

void free_transport_status(struct transport_status* status)
{
  free_pkts(status->sent);
  free_pkts(status->received);
  free_pkts(status->losses);
  free_pkts(status->duplicates);
  free_pkts(status->retransmitted);
}

void free_transport(struct transport* transport)
{
  free_fragment_buffers(transport->frag_buffer);
}

float bitrate(const struct transport_packet* pkts, struct timeval* since)
{
  const struct transport_packet* pkt;
  struct timeval last = *since;
  struct timeval period;
  float period_s;
  int total_len = 0;

  for(pkt = pkts; pkt != NULL; pkt = pkt->next) {
    if(timeval_compare(&(pkt->when), since) >= 0) {
      total_len += pkt->len;
      if(timeval_compare(&last, &(pkt->when)) == -1) last = pkt->when;
    }
  }

  timeval_subtract(&period, &last, since);
  period_s = (period.tv_sec*1000000 + period.tv_usec)/1000000;
  return (float)total_len/period_s;
}

int pkt_count(const struct transport_packet* pkts, struct timeval* since)
{
  const struct transport_packet* pkt;
  int pkt_number = 0;

  for(pkt = pkts; pkt != NULL; pkt = pkt->next)
  {
    if(timeval_compare(&(pkt->when), since) >= 0) {
      pkt_number++;
    }
  }
  return pkt_number;
}

int transport_on_send(struct transport_status* status, struct transport_packet* pkt)
{
  gettimeofday(&(pkt->when), 0);

  /* TODO check about fragments */
  if(!(status->sent) || (status->sent->seq_no < pkt->seq_no))
    add_pkt(&(status->sent), pkt);
  else
    add_pkt(&(status->retransmitted), pkt);

  return 0;
}

/* sent(s), received(r), losses(l), duplicated(d), retransmitted(t)
  status      r  r  r  l  r  r  d  r  r  l  r  l  r 
  seq_no      1  1  2     4  4  4  5  5     6     7
  frag_no     0  1  0     0  1  1  0  1     0     1
  frag_total  1  1  1     1  1  1  2  2     0     1

  Frag_no_gap = frag_no - last_frag_no if seq_no_gap == 0
  frag_no_gap = frag_no if seq_no_gap == 1
  frag_no_gap = frag_no if seq_no_gap > 1

  Received are: ((seq_no_gap == 1) and (last_total == last_frag) and (frag_no_gap == 0)) or ((seq_no_gap==0) and (frag_no_gap == 1))
  Duplicates are: (seq_no_gap < 0) or ((seq_no_gap == 0) and (frag_no_gap <= 0))
  Losses:
    - seq_no_gap == 1 and last_total != last_frag, lost = last_frag-last_total packets
    - seq_no_gap > 1 and last_total == last_frag, lost = gap - 1
    - seq_no_gap == 0 and frag_no_gap > 1, lost = frag_no_gap - 1

  Frag_total gives the number of fragments but it is 0 when there is one "fragment".
  Frag_no goes from 0 to frag_total-1
*/

int transport_on_received(struct transport_status* status, struct transport_packet* pkt)
{
  int i;
  int seq_no_gap = 1;
  int frag_no_gap = 0;
  int last_frag_total = 1;
  int last_frag = 0;
  int gap = 0; /* In case of losses, calculate this based on frag_no_gap and seq_no_gap combined */
  struct timeval start, stop, delta, current, now;

  gettimeofday(&now, 0);
  pkt->when = now;

  if( !pkt->frag_total ) pkt->frag_total++;
  if(status->received)
  {
    seq_no_gap = pkt->seq_no - status->received->seq_no;
    last_frag_total = status->received->frag_total;
    if( seq_no_gap == 0 ) frag_no_gap = pkt->frag_no - status->received->frag_no;
    else frag_no_gap = pkt->frag_no;

    last_frag = status->received->frag_no;
  }

  /* In-order no loss packet */
  log_debug(l, "last_frag_total/last_frag/frag_no_gap/seq_no_gap %d/%d/%d/%d.\n",
                last_frag_total, last_frag, frag_no_gap, seq_no_gap);
  if( ((seq_no_gap == 1) && ((last_frag_total-1) == last_frag) && (frag_no_gap == 0))
      || ((seq_no_gap == 0) && (frag_no_gap == 1)) ) {
    add_pkt(&(status->received), pkt);
    return 0;
  }

  /* Duplicate packet */
  if( ((seq_no_gap == 0) && (frag_no_gap == 0)) ){
    add_pkt(&(status->duplicates), pkt);
    return 1;
  }

  /* TODO I hope this formula is correct, gonna write some unit tests for it */
  gap += last_frag_total-1 - last_frag + frag_no_gap + seq_no_gap;
  log_debug(l, "The received packets gap is %d.\n", gap);
  /* Seq number gap */
  /* Example:
   *   1 is the last. 4 is the current. Gap is 3
   *   Add 2 packets: 2 and 3
   *   When of 2 is the same as 1
   *   When of 3 is the same as 4
   */
  if(gap > 1) {
    start = status->received->when;
    stop = now;
    timeval_subtract(&delta, &stop, &start);
    if(gap > 2) {
      delta.tv_sec = delta.tv_sec / (gap-2);
      delta.tv_usec = delta.tv_usec / (gap-2);
    }
    current = start;
    for(i = 1; i < gap; i++) {
      new_pkt(&(status->losses), status->received->seq_no + i, &current, 0, 0, 0); 
      timeval_add(&current, &current, &delta);
    }

    add_pkt(&(status->received), pkt);
    return 2;
  }

  /* Out of order packet */
  if(seq_no_gap < 1) {
    return 3;
  }

  /* Should not reach here */
  return -1;
}

int __transport_send(const struct transport* transport, struct transport_status* status,
                   struct transport_packet* pkt, const char* buf)
{
  int ret = add_outgoing_packet(transport, status, pkt, buf);
  return ret;
}

// int transport_send(struct transport* transport, struct transport_status* status,
//                    struct PRTP_packet* msg)
// {
//   char buf[BUF_SIZE + 1];
//   struct transport_packet pkt;
//   /* Use this ugly thing to save original data of update message and restore it */
//   void* save_ptr = NULL;
//   struct PRTP_packet* upd_msg;
//   int ret = 0, res = -1;

//   if (transport->skip_next) {
//     transport->skip_next = false;
//     return 0;
//   }

//   log_debug(l, "Tracing messages %d %s\n", msg->data.sensor_type, msg->data.sid);
//   pkt.seq_no = iotmsg_get_seq_no(msg);
//   pkt.next = NULL;
//   pkt.when.tv_sec = 0;
//   pkt.when.tv_usec = 0;
//   pkt.frag_no = 0;
//   pkt.frag_total = 1;

//   if( msg->type == UPDATE ) {
//     upd_msg = msg;
//     save_ptr = upd_msg->data.blob;

//     if( transport->frag_buffer->first_fragment )
//     {
//       free_fragment_list( transport->frag_buffer->first_fragment);
//       transport->frag_buffer->first_fragment = NULL;
//     }

//     fragment_update_message( transport->frag_buffer, upd_msg, 0 );
//     pkt.frag_total = iotmsg_get_frag_total(msg);
//     for( upd_msg->frag_no = 0; upd_msg->frag_no < upd_msg->frag_total; upd_msg->frag_no++ )
//     {
//       generate_update_messages( transport->frag_buffer, upd_msg );

//       pkt.frag_no = iotmsg_get_frag_no(msg);


//       pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );

//       res = __transport_send(transport, status, &pkt, buf);
//       if( upd_msg->frag_total > 1 ) free( upd_msg->data.blob );
//       if (res == -1) {
//         ret = res;
//         break;
//       }
//       else ret += res;
//     }
//     upd_msg->data.blob = save_ptr;
//     return ret;
//   }
//   else {
//     pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );
//     return __transport_send(transport, status, &pkt, buf);
//   }
// }
int transport_send(struct transport* transport, struct transport_status* status,
                   struct PRTP_packet* msg)
{
  char buf[BUF_SIZE + 1];
  struct transport_packet pkt;
  /* Use this ugly thing to save original data of update message and restore it */
  void* save_ptr = NULL;
  struct PRTP_packet* upd_msg;
  int ret = 0, res = -1;

  if (transport->skip_next) {
    transport->skip_next = false;
    return 0;
  }

  /* Fix: Access union members based on message type */
  if (msg->type == UPDATE) {
    log_debug(l, "Tracing messages %d %s\n", msg->data.update.sensor_type, msg->data.update.sid);
  }
  
  pkt.seq_no = iotmsg_get_seq_no(msg);
  pkt.next = NULL;
  pkt.when.tv_sec = 0;
  pkt.when.tv_usec = 0;
  pkt.frag_no = 0;
  pkt.frag_total = 1;

  if( msg->type == UPDATE ) {
    upd_msg = msg;
    save_ptr = upd_msg->data.update.blob.blob;  /* Fix: Access through union */

    if( transport->frag_buffer->first_fragment )
    {
      free_fragment_list( transport->frag_buffer->first_fragment);
      transport->frag_buffer->first_fragment = NULL;
    }

    fragment_update_message( transport->frag_buffer, upd_msg, 0 );
    pkt.frag_total = iotmsg_get_frag_total(msg);
    for( upd_msg->frag_no = 0; upd_msg->frag_no < upd_msg->frag_total; upd_msg->frag_no++ )
    {
      generate_update_messages( transport->frag_buffer, upd_msg );

      pkt.frag_no = iotmsg_get_frag_no(msg);

      pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );

      res = __transport_send(transport, status, &pkt, buf);
      if( upd_msg->frag_total > 1 ) free( upd_msg->data.update.blob.blob );  /* Fix */
      if (res == -1) {
        ret = res;
        break;
      }
      else ret += res;
    }
    upd_msg->data.update.blob.blob = save_ptr;  /* Fix: Restore through union */
    return ret;
  }
  else {
    pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );
    return __transport_send(transport, status, &pkt, buf);
  }
}

// enum MESSAGE_STATUS handle_fragmentation(struct transport* transport, struct PRTP_packet* upd_msg)
// {
//   struct fragment_buffer* frag_buf = transport->frag_buffer;
//   enum MESSAGE_STATUS upd_frag_status;

//     log_debug( l, "Coming to handle fragmentation %d\n", upd_msg->data.sensor_type);

//   if( upd_msg->frag_total == 1 ) return MESSAGE_STATUS_READY;

//   frag_buf = get_fragment_buffer(transport->frag_buffer, upd_msg->data.sid);
//   if (frag_buf == NULL) {
//       log_debug( l, "Got a fragmented message for %s not in the list of fragmentation buffers; message discarded\n", upd_msg->data.sid );
//       return MESSAGE_STATUS_ERROR;
//   }

//   upd_frag_status = update_fragment_buffer(frag_buf, upd_msg);
//   if (upd_frag_status == MESSAGE_STATUS_READY) {
//     log_debug(l, "%d message has size of %d bytes\n", upd_msg->seq_no, frag_buf_data_length(frag_buf));
//   } else {
//     log_debug( l, "Waiting for more fragments to arrive.\n");
//   }
//   return upd_frag_status;
// }
/* Fix 2: In handle_fragmentation() function around line 311 */
enum MESSAGE_STATUS handle_fragmentation(struct transport* transport, struct PRTP_packet* upd_msg)
{
  struct fragment_buffer* frag_buf = transport->frag_buffer;
  enum MESSAGE_STATUS upd_frag_status;

  /* Fix: Access through union for UPDATE message */
  log_debug( l, "Coming to handle fragmentation %d\n", upd_msg->data.update.sensor_type);

  if( upd_msg->frag_total == 1 ) return MESSAGE_STATUS_READY;

  frag_buf = get_fragment_buffer(transport->frag_buffer, upd_msg->data.update.sid);  /* Fix */
  if (frag_buf == NULL) {
      log_debug( l, "Got a fragmented message for %s not in the list of fragmentation buffers; message discarded\n", 
                upd_msg->data.update.sid );  /* Fix */
      return MESSAGE_STATUS_ERROR;
  }

  upd_frag_status = update_fragment_buffer(frag_buf, upd_msg);
  if (upd_frag_status == MESSAGE_STATUS_READY) {
    log_debug(l, "%d message has size of %d bytes\n", upd_msg->seq_no, frag_buf_data_length(frag_buf));
  } else {
    log_debug( l, "Waiting for more fragments to arrive.\n");
  }
  return upd_frag_status;
}
int transport_receive(struct transport* transport, struct PRTP_packet** msg,
                      struct sockaddr_storage* from, socklen_t* fromlen)
{
  /* Adding terminating NULL-character for printing */
  char buf[BUF_SIZE + 1];
  size_t recvlen;
  enum MESSAGE_STATUS frag_status;
  
  recvlen = recvfrom(transport->sd, buf, BUF_SIZE, 0, (struct sockaddr*)from, fromlen);
  if (recvlen == 0) {
      log_error(l, "Connection is closed.\n");
      return -1;
  }
  else if (recvlen == -1) {
      log_error(l, "Recvfrom failed.\n");
      return -1;
  }
  log_debug(l, "Received %u bytes\n", recvlen);

  /* Make sure msg is NULL when parsing fails */
  if( parse_bson_document(buf, recvlen, msg) == -1 ) *msg = NULL;

  /* Fragmentation reassembly here */
  if( (**msg).type == UPDATE ) {
    frag_status = handle_fragmentation(transport, *msg);
    if( frag_status != MESSAGE_STATUS_READY ) {
      free_iotmsg(*msg);
      *msg = NULL;
    }
  }

  return recvlen;
}

bool transport_retransmit(struct transport_status* status)
{
  struct timeval tv, tvelapsed, tvremain;

  /* If nothing was sent yet, no point in retransmission:) */
  if( !(status->sent) ) return false;

  /* If nothing was received but something was sent OR
   * if last received seq_no is less than last sent seq_no
   * then check the timer */
  if( ( !(status->received) && (status->sent) ) ||
      ( status->received->seq_no < status->sent->seq_no) )
  {
    gettimeofday(&tv, 0);

    if( !timeval_subtract( &tvelapsed, &tv, &(status->sent->when) ) ) {
      /* if timeout - elapsed is negative, send retransmission */
      if( timeval_subtract( &tvremain, &tv_retransmit, &tvelapsed) ) {
        return true;
      }
    }
  }
  return false;
}
