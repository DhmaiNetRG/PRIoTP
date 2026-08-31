#include "transport.h"
#include "utils.h"
#include "messages.h"
#include "bson_msg.h"
#include "client_module.h"
#include "logger.h"
#include "bson_parser.h"
#include "security.h"   /* PRIoTPS Phase 4: ASCON-AEAD128 encrypt on egress */
#include "telemetry.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#undef BUF_SIZE
#define BUF_SIZE 1600

static const struct timeval tv_retransmit = {RETRANSMIT_TIMEOUT/1000, (1000*RETRANSMIT_TIMEOUT)};

static struct logger* l = NULL;

void init_transport()
{
  srand (time(NULL));
  l = init_logger(stdout, stderr, stderr, "Transport");
  init_congestion_control();

  /*
   * PRIoTPS Phase 4: Initialize the global security context.
   * Loads PSK table from psk.conf.  If the file is absent or malformed,
   * init_security_ctx() logs an error and leaves the context disabled;
   * all transmit paths below then fall back to the original plaintext path,
   * so the daemon continues operating without encryption.
   */
  init_security_ctx(NULL);  /* PRIoTPS: loads or generates identity from conf/priotps_identity.bin */
}

void shutdown_transport()
{
  shutdown_security_ctx(); /* PRIoTPS Phase 4: zero key material before exit */
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

  /*
   * PRIoTPS Phase 4: secured output buffer.
   *
   * Maximum size = plaintext (BUF_SIZE+1) + SEH (12 B) + Auth Tag (16 B).
   * ASCON produces zero ciphertext expansion, so:
   *   secured_buf size = (BUF_SIZE + 1) + PRIOTPS_MAX_OVERHEAD
   *
   * This is a stack allocation; add_outgoing_packet() copies the bytes
   * into heap memory before transport_send() returns, so it is safe.
   */
  char secured_buf[BUF_SIZE + 1 + PRIOTPS_MAX_OVERHEAD];
  size_t secured_len = 0;

  struct transport_packet pkt;
  /* Use this ugly thing to save original data of update message and restore it */
  void* save_ptr = NULL;
  struct PRTP_packet* upd_msg;
  priotps_security_ctx_t *sec_ctx;  /* PRIoTPS Phase 2 */
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
    iotmsg_update_fragmented(msg, pkt.frag_total > 1);

    for( upd_msg->frag_no = 0; upd_msg->frag_no < upd_msg->frag_total; upd_msg->frag_no++ )
    {
      generate_update_messages( transport->frag_buffer, upd_msg );

      pkt.frag_no = iotmsg_get_frag_no(msg);

      /* Serialize BSON into the plaintext buffer */
      pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );

      /*
       * PRIoTPS Phase 4 — UPDATE fragment encrypt path.
       *
       * After serialize_iotmsg() writes the BSON into buf[0..pkt.len-1],
       * encrypt it in-place into secured_buf as:
       *
       *   secured_buf = [12-byte SEH] [pkt.len-byte ciphertext] [16-byte tag]
       *
       * The SEH carries ver, algo_id, key_id, flags, and a nonce derived
       * from msg->seq_no, msg->timestamp, msg->frag_no, msg->frag_total.
       * The SEH is also the AEAD Associated Data (AAD), so any tampering
       * with the header will fail the authentication check on the receiver.
       *
       * pkt.len is updated to secured_len so that add_outgoing_packet()
       * copies exactly the right number of bytes into its heap buffer,
       * and transmit_pkt() passes the correct length to sendto().
       *
       * If security is disabled (psk.conf not found at startup), the
       * original buf/pkt.len are used unchanged — full backward compat.
       */
      sec_ctx = get_priotps_security_ctx();
      if (sec_ctx->initialized && sec_ctx->sessions.entries[0].active) {
        uint32_t sid = sec_ctx->sessions.entries[0].session_id;
        if (security_core_encrypt(sec_ctx, sid,
                                   (const uint8_t *)buf, (size_t)pkt.len,
                                   (uint8_t *)secured_buf, &secured_len) == 0) {
          telemetry_encrypt(sid, (size_t)pkt.len);
          pkt.len = (int)secured_len;
          res = __transport_send(transport, status, &pkt, secured_buf);
        } else {
          log_error(l, "PRIoTPS: encrypt failed for UPDATE frag %u/%u;"
                       " dropping fragment.\n",
                    upd_msg->frag_no, upd_msg->frag_total);
          res = -1;
        }
      } else {
        /* No session yet (handshake pending) — transmit plaintext */
        res = __transport_send(transport, status, &pkt, buf);
      }
      /* --- end PRIoTPS Phase 4 UPDATE path --- */

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
    /* Serialize BSON into the plaintext buffer */
    pkt.len = serialize_iotmsg( msg, buf, BUF_SIZE + 1 );

    /*
     * PRIoTPS Phase 4 — control-message encrypt path.
     *
     * LIST, SUBSCRIBE, SUBSCRIBE_ACK, UPDATE_ACK, UPDATE_NACK,
     * KEEP_ALIVE, UNSUBSCRIBE all land here.  Same encryption contract
     * as the UPDATE path above; see that block for the full rationale.
     *
     * For non-UPDATE messages frag_no == 0 and frag_total == 1, so the
     * nonce reduces to:
     *   nonce = (seq_no & 0xFFFF) << 48 | (timestamp & 0xFFFF) << 32
     *
     * which is still unique per-packet as long as seq_no is incremented.
     */
    sec_ctx = get_priotps_security_ctx();
    if (sec_ctx->initialized && sec_ctx->sessions.entries[0].active) {
      uint32_t sid = sec_ctx->sessions.entries[0].session_id;
      if (security_core_encrypt(sec_ctx, sid,
                                 (const uint8_t *)buf, (size_t)pkt.len,
                                 (uint8_t *)secured_buf, &secured_len) == 0) {
        telemetry_encrypt(sid, (size_t)pkt.len);
        pkt.len = (int)secured_len;
        return __transport_send(transport, status, &pkt, secured_buf);
      }
      log_error(l, "PRIoTPS: encrypt failed for control message type %u;"
                   " dropping packet.\n", msg->type);
      return -1;
    }
    /* --- end PRIoTPS Phase 4 control path --- */

    /* No session yet — transmit plaintext */
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

  if (upd_msg->data.update.sid == NULL) {
    log_debug( l, "Fragmented message has NULL sid; message discarded\n");
    return MESSAGE_STATUS_ERROR;
  }

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
  /*
   * Wire buffer: holds the raw UDP payload as received from the network.
   *
   * After Phase 4, all PRIoTPS-enabled peers transmit:
   *   [12-byte SEH] [BSON ciphertext] [16-byte auth tag]
   *
   * The maximum secured datagram is:
   *   BUF_SIZE (BSON) + PRIOTPS_SEH_SIZE (12) + PRIOTPS_TAG_SIZE (16) = 1628 bytes
   *
   * We allocate BUF_SIZE + 1 + PRIOTPS_MAX_OVERHEAD to accommodate it.
   */
  char buf[BUF_SIZE + 1 + PRIOTPS_MAX_OVERHEAD];

  /*
   * Plaintext buffer: receives the decrypted BSON from priotps_decrypt().
   * Sized for BUF_SIZE + 1 to match the existing parse_bson_document() contract.
   * Declared separately so the security and parsing layers never share memory.
   */
  char plain_buf[BUF_SIZE + 1];

  size_t recvlen;
  size_t plain_len = 0;
  enum MESSAGE_STATUS frag_status;
  priotps_security_ctx_t *sec_ctx;
  uint32_t session_id_out = 0;
  int decrypt_ret;

  /*
   * Step 1 — Receive raw UDP datagram.
   *
   * recvfrom() writes the raw bytes (SEH + ciphertext + tag when secured,
   * or plain BSON when security is disabled) into buf[].
   *
   * Note: the BUF_SIZE argument caps the bytes read to BUF_SIZE, which is
   * the maximum BSON size we process.  The extra PRIOTPS_MAX_OVERHEAD bytes
   * in the declaration are consumed by the SEH and tag fields that sit
   * outside the BSON payload; recvfrom() will read them into the same buffer.
   */
  recvlen = recvfrom(transport->sd, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr*)from, fromlen);
  if (recvlen == 0) {
      log_error(l, "Connection is closed.\n");
      return -1;
  }
  else if (recvlen == (size_t)-1) {
      log_error(l, "Recvfrom failed.\n");
      return -1;
  }
  log_debug(l, "Received %u bytes\n", recvlen);

  /*
   * Step 2 — PRIoTPS packet classification and dispatch.
   */
  sec_ctx = get_priotps_security_ctx();

  if (sec_ctx->initialized && recvlen >= 1) {
    uint8_t flags = seh_get_flags((const uint8_t *)buf, recvlen);

    /* 2.1 — Handshake response packet from server (SEH_FLAG_HSK = 0) */
    /* Minimum wire size: SEH_HSK_OVERHEAD (41) + ECIES_OVERHEAD*2 (48) + INNER_PAYLOAD (20) = 109 bytes */
    if (flags == SEH_FLAG_HSK && recvlen >= 109) {
      uint8_t session_key[16];
      uint8_t server_pub[32];
      uint32_t session_id = 0;

      if (security_core_client_process_handshake(sec_ctx,
                                                 (const uint8_t *)buf, recvlen,
                                                 session_key, server_pub,
                                                 &session_id) == 0) {
        log_print(l, "PRIoTPS: Handshake response received from server. Session ID: %u\n", session_id);

        /* Register the established session in client session table */
        priotps_session_t *sess = security_core_add_client_session(sec_ctx,
                                                                   server_pub,
                                                                   session_key,
                                                                   session_id);
        if (sess) {
          log_debug(l, "PRIoTPS: Session %u successfully registered on client.\n", session_id);
        }

        /* Generate compact Handshake ACK: [flag(1)][session_id(4 BE)] = 5 bytes */
        uint8_t ack_pkt[16];  /* compact ACK is 5 bytes; extra room for safety */
        size_t ack_len = sizeof(ack_pkt);
        if (security_core_client_build_ack(sec_ctx, session_id, ack_pkt, &ack_len) == 0) {
          /* Send ACK to server */
          ssize_t sent = -1;
          if (from != NULL && fromlen != NULL && *fromlen > 0) {
            sent = sendto(transport->sd, (const char *)ack_pkt, ack_len, 0,
                          (struct sockaddr *)from, *fromlen);
            if (sent < 0 && errno == EISCONN) {
                sent = send(transport->sd, (const char *)ack_pkt, ack_len, 0);
            }
          } else if (transport->addr_len > 0) {
            sent = sendto(transport->sd, (const char *)ack_pkt, ack_len, 0,
                          (struct sockaddr *)&transport->addr, transport->addr_len);
            if (sent < 0 && errno == EISCONN) {
                sent = send(transport->sd, (const char *)ack_pkt, ack_len, 0);
            }
          } else {
            sent = send(transport->sd, (const char *)ack_pkt, ack_len, 0);
          }

          if (sent < 0) {
            log_error(l, "PRIoTPS: Failed to transmit Handshake ACK to server (errno: %d, %s).\n",
                      errno, strerror(errno));
          } else {
            log_print(l, "PRIoTPS: Handshake ACK sent to server (%zu bytes, session=%u).\n", ack_len, session_id);
          }
        }


        *msg = NULL;
        return (int)recvlen;
      }
      /*
       * Handshake parsing failed on this packet.
       * Do NOT consume or drop: fall through to the plaintext BSON parser below
       * to ensure legacy / plaintext packets starting with byte & 0x03 == 0 are preserved.
       */
      log_debug(l, "PRIoTPS: Packet matched SEH_FLAG_HSK but failed handshake processing; falling back to plaintext BSON.\n");
    }

    /* 2.2 — Handshake confirmation ACK (SEH_FLAG_ACK = 2) */
    if (flags == SEH_FLAG_ACK) {
      log_debug(l, "PRIoTPS: Handshake ACK received (client ignore).\n");
      *msg = NULL;
      return (int)recvlen;
    }

    /* 2.3 — Encrypted application data packet (SEH_FLAG_ENC = 1) */
    if (flags == SEH_FLAG_ENC) {
      decrypt_ret = security_core_decrypt(sec_ctx,
                                           (const uint8_t *)buf, recvlen,
                                           (uint8_t *)plain_buf, &plain_len,
                                           &session_id_out);

      if (decrypt_ret != 0) {
        telemetry_security_event("DecryptFail", session_id_out, "AEAD tag mismatch");
        log_error(l, "PRIoTPS: Inbound encrypted packet authentication/decryption failed.\n");
        *msg = NULL;
        return -1;
      }

      log_debug(l, "PRIoTPS: Decrypted %u -> %zu BSON bytes (session %u).\n",
                recvlen, plain_len, session_id_out);
      telemetry_decrypt(session_id_out, plain_len);

      /* Step 3 — Parse the decrypted BSON document with buffer boundary verification */
      if (plain_len < sizeof(plain_buf)) {
        plain_buf[plain_len] = '\0';
        if (parse_bson_document(plain_buf, plain_len, msg) == -1) {
          *msg = NULL;
        }
      } else {
        telemetry_security_event("MalformedSEH", 0, "decrypted payload exceeds buffer");
        log_error(l, "PRIoTPS: Decrypted payload length (%zu) exceeds plain_buf capacity (%zu).\n",
                  plain_len, sizeof(plain_buf));
        *msg = NULL;
        return -1;
      }
    } else {
      /* 2.4 — Unknown flag / not valid SEH / failed HSK: fall back to plaintext BSON */
      if (recvlen < sizeof(buf)) {
        buf[recvlen] = '\0';
      } else {
        buf[sizeof(buf) - 1] = '\0';
      }
      if (parse_bson_document(buf, recvlen, msg) == -1) {
        *msg = NULL;
      }
    }
  } else {
    /*
     * Security disabled or empty packet — plaintext path (backward compatibility).
     * buf already holds the raw BSON; pass it straight to the parser.
     */
    if (recvlen < sizeof(buf)) {
      buf[recvlen] = '\0';
    } else {
      buf[sizeof(buf) - 1] = '\0';
    }
    if (parse_bson_document(buf, recvlen, msg) == -1) {
      *msg = NULL;
    }
  }

  /*
   * Step 4 — Fragmentation reassembly (unchanged).
   *
   * If msg was successfully parsed and is an UPDATE, hand it to
   * handle_fragmentation().  This logic is identical to the original
   * and operates purely on the parsed PRTP_packet struct, so it is
   * agnostic to whether the payload arrived encrypted or in plaintext.
   */
  if (*msg != NULL && (**msg).type == UPDATE) {
    frag_status = handle_fragmentation(transport, *msg);
    if (frag_status != MESSAGE_STATUS_READY) {
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
