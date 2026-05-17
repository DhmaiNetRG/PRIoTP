#include <stdlib.h>
#include <check.h>
#include <unistd.h>
#include "../src/transport.h"
#include "../src/logger.h"
#include "../src/sensor_types.h"

void
setup (void)
{
  init_transport();
}

void
teardown (void)
{
  shutdown_transport();
}

START_TEST (test_transport_basic)
{
  int i;
  int pkts = 10;
  struct transport_packet pkt;
  struct transport_packet ack;
  int pkt_len = 100;
  int ack_len = 20;
  int rtt_usec = 500;
  float pkt_bitrate = pkt_len/(rtt_usec*1000000);
  float ack_bitrate = ack_len/(rtt_usec*1000000);
  float epsilon = 0.00001;
  struct transport_status t_status = {NULL, NULL, NULL, NULL, NULL};
  struct timeval start;

  pkt.len = pkt_len;
  ack.len = ack_len;
  pkt.frag_total = pkt.frag_no = ack.frag_total = ack.frag_no = 0;
  gettimeofday(&start, 0);
  for( i = 0; i < pkts; i++ )
  {
    pkt.seq_no = i;
    transport_on_send(&t_status, &pkt);
    usleep(rtt_usec);

    ack.seq_no = i;
    transport_on_received(&t_status, &ack);
  }

  fail_unless ( pkt_count(t_status.sent, &start) == pkts,
      "Not all packets sent." );
  fail_unless ( pkt_count(t_status.received, &start) == pkts,
      "Not all acks received." );

  fail_unless ( pkt_count(t_status.losses, &start) == 0,
      "Some losses where should not be." );
  fail_unless ( pkt_count(t_status.duplicates, &start) == 0,
      "Some duplicates where should not be." );
  fail_unless ( pkt_count(t_status.retransmitted, &start) == 0,
      "Some retransmissions where should not be." );

  fail_unless ( abs(bitrate(t_status.sent, &start) - pkt_bitrate) < epsilon,
      "Error in packets bitrate calculation." );
  fail_unless ( abs(bitrate(t_status.received, &start) - ack_bitrate) < epsilon,
      "Error in acks bitrate calculation." );

  free_transport_status(&t_status);
}
END_TEST

START_TEST (test_losses)
{
  int i;
  int loss_rate = 2;
  int pkts = 10;
  struct transport_packet pkt;
  struct transport_packet ack;
  int pkt_len = 100;
  int ack_len = 20;
  int rtt_usec = 500;
  int sent_counter = 0;
  int lost_counter = 0;
  float pkt_bitrate = pkt_len/(loss_rate*rtt_usec*1000000);
  float ack_bitrate = ack_len/(loss_rate*rtt_usec*1000000);
  float epsilon = 0.00001;
  struct transport_status t_status = {NULL, NULL, NULL, NULL, NULL};
  struct timeval start;

  pkt.len = pkt_len;
  ack.len = ack_len;
  pkt.frag_total = pkt.frag_no = ack.frag_total = ack.frag_no = 0;
  gettimeofday(&start, 0);
  for( i = 0; i < pkts; i+=loss_rate )
  {
    pkt.seq_no = i;
    transport_on_send(&t_status, &pkt);
    usleep(rtt_usec);

    ack.seq_no = i;
    transport_on_received(&t_status, &ack);
    sent_counter++;
    lost_counter += loss_rate-1;
  }
  /* Last losses will not be noticed */
  lost_counter -= loss_rate-1;

  fail_unless ( pkt_count(t_status.sent, &start) == sent_counter,
      "Not all packets sent." );
  fail_unless ( pkt_count(t_status.received, &start) == sent_counter,
      "Not all acks received." );

  fail_unless ( pkt_count(t_status.losses, &start) == lost_counter,
      "Lost packets are not counted right." );
  fail_unless ( pkt_count(t_status.duplicates, &start) == 0,
      "Some duplicates where should not be." );
  fail_unless ( pkt_count(t_status.retransmitted, &start) == 0,
      "Some retransmissions where should not be." );

  fail_unless ( abs(bitrate(t_status.sent, &start) - pkt_bitrate) < epsilon,
      "Error in packets bitrate calculation." );
  fail_unless ( abs(bitrate(t_status.received, &start) - ack_bitrate) < epsilon,
      "Error in acks bitrate calculation." );

  free_transport_status(&t_status);
}
END_TEST

START_TEST (test_retransmissions)
{
  int i;
  int pkts = 10;
  struct transport_packet pkt;
  struct transport_packet ack;
  int pkt_len = 100;
  int ack_len = 20;
  int rtt_usec = 500;
  int retransmit_usec = 500;
  int sent_counter = 0;
  int retransmit_counter = 0;
  float pkt_bitrate = pkt_len/((rtt_usec+retransmit_usec)*1000000);
  float ack_bitrate = ack_len/((rtt_usec+retransmit_usec)*1000000);
  float epsilon = 0.00001;
  struct transport_status t_status = {NULL, NULL, NULL, NULL, NULL};
  struct timeval start;

  pkt.len = pkt_len;
  ack.len = ack_len;
  pkt.frag_total = pkt.frag_no = ack.frag_total = ack.frag_no = 0;
  gettimeofday(&start, 0);
  for( i = 0; i < pkts; i++ )
  {
    pkt.seq_no = i;
    transport_on_send(&t_status, &pkt);
    usleep(retransmit_usec);
    transport_on_send(&t_status, &pkt);
    usleep(rtt_usec);

    ack.seq_no = i;
    transport_on_received(&t_status, &ack);
    sent_counter++;
    retransmit_counter++;
  }

  fail_unless ( pkt_count(t_status.sent, &start) == sent_counter,
      "Not all packets sent." );
  fail_unless ( pkt_count(t_status.received, &start) == sent_counter,
      "Not all acks received." );

  fail_unless ( pkt_count(t_status.losses, &start) == 0,
      "Some losses where should not be." );
  fail_unless ( pkt_count(t_status.duplicates, &start) == 0,
      "Some duplicates where should not be." );
  fail_unless ( pkt_count(t_status.retransmitted, &start) == retransmit_counter,
      "Retransmissions are not counted right." );

  fail_unless ( abs(bitrate(t_status.sent, &start) - pkt_bitrate) < epsilon,
      "Error in packets bitrate calculation." );
  fail_unless ( abs(bitrate(t_status.received, &start) - ack_bitrate) < epsilon,
      "Error in acks bitrate calculation." );

  free_transport_status(&t_status);
}
END_TEST

START_TEST (test_duplicates)
{
  int i;
  int pkts = 10;
  struct transport_packet pkt;
  struct transport_packet ack;
  int pkt_len = 100;
  int ack_len = 20;
  int rtt_usec = 500;
  int sent_counter = 0;
  int duplicate_counter = 0;
  float pkt_bitrate = pkt_len/(rtt_usec*1000000);
  float ack_bitrate = ack_len/(rtt_usec*1000000);
  float epsilon = 0.00001;
  struct transport_status t_status = {NULL, NULL, NULL, NULL, NULL};
  struct timeval start;

  pkt.len = pkt_len;
  ack.len = ack_len;
  pkt.frag_total = pkt.frag_no = ack.frag_total = ack.frag_no = 0;
  gettimeofday(&start, 0);
  for( i = 0; i < pkts; i++ )
  {
    pkt.seq_no = i;
    transport_on_send(&t_status, &pkt);
    usleep(rtt_usec);

    ack.seq_no = i;
    transport_on_received(&t_status, &ack);
    transport_on_received(&t_status, &ack);

    sent_counter++;
    duplicate_counter++;
  }

  fail_unless ( pkt_count(t_status.sent, &start) == sent_counter,
      "Not all packets sent." );
  fail_unless ( pkt_count(t_status.received, &start) == sent_counter,
      "Not all acks received." );

  fail_unless ( pkt_count(t_status.losses, &start) == 0,
      "Some losses where should not be." );
  fail_unless ( pkt_count(t_status.duplicates, &start) == duplicate_counter,
      "Duplicates are not counted right." );
  fail_unless ( pkt_count(t_status.retransmitted, &start) == 0,
      "Some retransmissions where should not be." );

  fail_unless ( abs(bitrate(t_status.sent, &start) - pkt_bitrate) < epsilon,
      "Error in packets bitrate calculation." );
  fail_unless ( abs(bitrate(t_status.received, &start) - ack_bitrate) < epsilon,
      "Error in acks bitrate calculation." );

  free_transport_status(&t_status);
}
END_TEST

Suite *
transport_suite(void)
{
  Suite *s = suite_create ("Transport module");

  TCase *tc_core = tcase_create ("Core");

  /* Core test case */
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_transport_basic);
  tcase_add_test (tc_core, test_losses);
  tcase_add_test (tc_core, test_retransmissions);
  tcase_add_test (tc_core, test_duplicates);
  suite_add_tcase (s, tc_core);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = transport_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

