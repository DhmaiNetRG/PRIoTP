#include <stdlib.h>
#include <check.h>
#include <unistd.h>
#include "../src/fragment_buffer.h"
#include "../src/logger.h"
#include "../src/messages.h"
#include "../src/sensor_types.h"
#include "../src/void_data.h"

static struct fragment_buffer* frag_buf = NULL;
static char cam_data[] = "\
       1 00000400 000000      000..................................................................\n\
       1 00000400 000000      100..................................................................\n\
       1 00000400 000000      200..................................................................\n\
       1 00000400 000000      300.................................................................\n";

static struct void_data data = {399, cam_data};

#define MAX_DATA 100

void
setup (void)
{
  create_fragment_buffer("none", &frag_buf);
}

void
teardown (void)
{
  free_fragment_buffers(frag_buf);
}

START_TEST (test_fragmentation)
{
  struct PRTPmsg* msg = create_PRTPmsg(UPDATE);
  struct PRTPmsg_update* upd_msg = (struct PRTPmsg_update*)msg;
  void* save_ptr;

  upd_msg->seq_no = 1;
  upd_msg->reliable = false;
  PRTPmsg_set_sid( msg, "none" );
  upd_msg->sensor_type = CAMERA;
  upd_msg->frag_no = 0;
  upd_msg->frag_total = 0;

  fail_if( PRTPmsg_set_data( msg, &data ) == -1, "Error setting message data!" );

  save_ptr = upd_msg->data.blob;
  fragment_update_message( frag_buf, upd_msg, MAX_DATA );
  fail_unless( upd_msg->frag_total == 4, "Wrong number of fragments, should be 4." );
  for( upd_msg->frag_no = 0; upd_msg->frag_no < upd_msg->frag_total-1; upd_msg->frag_no++ )
  {
    generate_update_messages( frag_buf, upd_msg );
    fail_if(upd_msg->data.len != MAX_DATA, "Wrong data length in the fragment.");
    fail_unless(((char*)(upd_msg->data.blob))[30]-'0' == upd_msg->frag_no, "Wrong data in the fragment.");
  }
  /* Last fragment */
  generate_update_messages( frag_buf, upd_msg );
  fail_unless( upd_msg->data.len == 99, "Wrong data length in the last fragment.");

  free_fragment_list( frag_buf->first_fragment );
  frag_buf->first_fragment = NULL;
  upd_msg->data.blob = save_ptr;
  free_PRTPmsg(msg);
}
END_TEST

Suite *
fragmentation_suite(void)
{
  Suite *s = suite_create ("Fragmentation module");

  TCase *tc_core = tcase_create ("Core");

  /* Core test case */
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_fragmentation);
  suite_add_tcase (s, tc_core);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = fragmentation_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

