#include <stdlib.h>
#include <check.h>
#include "../src/clients.h"
#include "../src/utils.h"
#include "../src/logger.h"

extern void add_client(const struct sockaddr_storage* addr, socklen_t len);
static struct logger* l;

void
setup (void)
{
  l = init_logger(stdout, stderr, stderr, "Clients check");
  init_clients();
}

void
teardown (void)
{
  shutdown_logger(l);
  shutdown_clients();
}

START_TEST (test_clients_expiration)
{
  struct sockaddr_in addr;
  int expired;

  addr.sin_port = 1;
  add_client((struct sockaddr_storage*)&addr, sizeof(struct sockaddr_in));
  addr.sin_port = 2;
  add_client((struct sockaddr_storage*)&addr, sizeof(struct sockaddr_in));

  sleep(KEEP_ALIVE_TIMEOUT/1000 + 1);
  expired = prune_expired_clients();
  log_debug(l, "Expired %d clients\n", expired);
  fail_unless(expired == 2,
      "Error pruning expired clients");
}
END_TEST

Suite *
clients_suite(void)
{
  Suite *s = suite_create ("Clients");

  TCase *tc_core = tcase_create ("Core");

  /* Core test case */
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_clients_expiration);
  tcase_set_timeout (tc_core, KEEP_ALIVE_TIMEOUT/1000 + 2 );
  suite_add_tcase (s, tc_core);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = clients_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

