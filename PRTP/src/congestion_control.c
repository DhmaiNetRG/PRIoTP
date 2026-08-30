#include "transport.h"
#include "utils.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

static struct logger* l = NULL;
void init_congestion_control()
{
  l = init_logger(stdout, stderr, stderr, "Congestion Control");
}

void shutdown_congestion_control()
{
  shutdown_logger(l);
}

int transmit_pkt(const struct transport* transport, struct transport_status* status,
                 struct transport_packet* pkt, const char* buf);

int schedule_pkt(struct transport_status* status, struct transport_packet* pkt)
{
  struct timeval delay;
  int genRandom;

  delay.tv_sec = 0;
  delay.tv_usec = status->sched.delay_msecs*1000;
  gettimeofday(&(pkt->when), 0);

  if( status->sched.delay_msecs )
    timeval_add( &(pkt->when), &(pkt->when), &delay );

  if( status->sched.p_prob > 0.00001 )
  {
    genRandom = rand() % 100 + 1;
    if( status->sched.lossy_state )
    {
      if( genRandom < status->sched.q_prob * 100 )
        status->sched.lossy_state = 0;
    }
    else
      if( genRandom < status->sched.p_prob * 100 )
        status->sched.lossy_state = 1;
  }

  if( status->sched.lossy_state ) {
    pkt->when.tv_sec = 0;
    pkt->when.tv_usec = 0;
    return 0;
  }

  /* TODO Implement Congestion Control here:) */
  /* For example, small messages are scheduled right away */
  /* 150 bytes are enough for all non-camera sensors */

  if( pkt->len < 150 ) return 0;

  return 0;
}

int add_outgoing_packet(const struct transport* transport, struct transport_status* status,
                        struct transport_packet* outgoing_pkt, const char* buf)
{
  int ret = -1;
  struct transport_packet* pkt;
  // struct transport_packet* prev = NULL;
  struct timeval tv, tmp;
  struct transport_packet* new_pkt;

  if( !status ) return transmit_pkt(transport, status, outgoing_pkt, buf);

  pkt = status->sched.outgoing;

  /* old packets already scheduled at some point */
  while( pkt != NULL )
  {
    if( (pkt->seq_no == outgoing_pkt->seq_no) &&
        (pkt->frag_no == outgoing_pkt->frag_no) ) {
      /* no need to do anything */ 
      log_debug(l, "Not rescheduling duplicate packet %u/%u/%u.\n",
                   outgoing_pkt->seq_no, outgoing_pkt->frag_no, outgoing_pkt->frag_total);
      ret = 0;
    }

    /*if( ((pkt->seq_no == outgoing_pkt->seq_no) &&
        (pkt->frag_no < outgoing_pkt->frag_no)) ||
        (pkt->seq_no < outgoing_pkt->seq_no) ) {
      log_debug(l, "Cancelling outdated transmission with %u/%u seq_no/frag_no by new %u/%u.\n",
                   pkt->seq_no, pkt->frag_no, outgoing_pkt->seq_no, outgoing_pkt->frag_no);

      if( prev ) prev->next = pkt->next;
      else status->sched.outgoing = pkt->next;

      free(((struct data_packet*)pkt)->buf);
      free(pkt);

      if( prev ) pkt = prev->next; else pkt = status->sched.outgoing;
      if( pkt == NULL ) break; else continue;
    }
    */
    
    // prev = pkt;
    pkt = pkt->next;
  }
  
  if( ret == 0 ) return ret;

  /* calculate the interval at when to schedule the packet */
  schedule_pkt(status, outgoing_pkt);

  /* Packet lost */
  if( !outgoing_pkt->when.tv_sec && !outgoing_pkt->when.tv_usec )
  {
    log_debug(l, "Packet lost by the loss generator.\n");
    transport_on_send(status, outgoing_pkt);
    return 0;
  }

  gettimeofday(&tv, 0);
  /* If scheduled now or close to now (withing 1 ms) transmit right away */
  if( timeval_subtract( &tmp, &(outgoing_pkt->when), &tv ) ||
      ((tmp.tv_sec == 0) && (tmp.tv_usec < 1000)) ) {
    log_debug(l, "Transmitting the packet without using the queue.\n");
    ret = transmit_pkt(transport, status, outgoing_pkt, buf);
    return ret;
  }

  /* Finally, put the packet in the queue */
  log_debug(l, "Adding the packet (seq,frag,total) %u/%u/%u to the queue.\n",
               outgoing_pkt->seq_no, outgoing_pkt->frag_no, outgoing_pkt->frag_total);

  new_pkt = xalloc(sizeof(struct data_packet));
  ((struct data_packet*)new_pkt)->buf = xalloc(outgoing_pkt->len);
  memcpy(((struct data_packet*)new_pkt)->buf, buf, outgoing_pkt->len);

  new_pkt->seq_no = outgoing_pkt->seq_no;
  new_pkt->when = outgoing_pkt->when;
  new_pkt->len = outgoing_pkt->len;
  new_pkt->frag_no = outgoing_pkt->frag_no;
  new_pkt->frag_total = outgoing_pkt->frag_total;

  new_pkt->next = status->sched.outgoing;
  status->sched.outgoing = new_pkt;

  return 0;
}

int transmit_from_queue(struct transport* transport, struct transport_status* status,
                        struct timeval* next_event)
{
  int ret = -1;
  struct transport_packet* pkt, *prev = NULL;
  struct timeval tv;
  struct timeval tmp;

  /* TODO calculate next event properly */
  struct timeval __stub__next_event = {0, 200000};

  if( !status ) return -1;

  prev = NULL;
  pkt = status->sched.outgoing;

  gettimeofday(&tv, 0);

  while( pkt != NULL ) {
    /* those that time passed, or those that are very close to now (1ms) */ 
    if( timeval_subtract( &tmp, &(pkt->when), &tv ) ||
        ((tmp.tv_sec == 0) && (tmp.tv_usec < 1000)) )
    {
      log_debug(l, "Transmitting from the queue %u/%u/%u.\n",
                   pkt->seq_no, pkt->frag_no, pkt->frag_total);
      ret = transmit_pkt(transport, status, pkt, ((struct data_packet*)pkt)->buf);

      if( prev ) prev->next = pkt->next;
      else status->sched.outgoing = pkt->next;

      free(((struct data_packet*)pkt)->buf);
      free(pkt);

      if( prev ) pkt = prev->next; else pkt = status->sched.outgoing;
      if( pkt == NULL ) break; else continue;
    }
    prev = pkt;
    pkt = pkt->next;
  }

  timeval_add(next_event, &tv, &__stub__next_event);
  return ret;
}


int transmit_pkt(const struct transport* transport, struct transport_status* status,
                 struct transport_packet* pkt, const char* buf)
{
  int ret;
  ret = sendto(transport->sd, buf, pkt->len, 0, (struct sockaddr*)&(transport->addr), transport->addr_len);
  if( (ret != -1) && status) transport_on_send(status, pkt);
  return ret;
}
