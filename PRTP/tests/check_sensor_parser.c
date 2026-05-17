#include <stdlib.h>
#include <check.h>
#include "../src/sensor_parser.h"
#include "../src/logger.h"

struct sensor* sensor;

void
setup (void)
{
  sensor = malloc( sizeof(struct sensor) );
  init_sensor_parser();
}

void
teardown (void)
{
  free( sensor );
  shutdown_sensor_parser();
}

void compare_sensors(struct sensor* right)
{
  fail_unless (sensor->type == right->type, "Wrong sensor type");

  fail_unless (strcmp(sensor->id, right->id) == 0, "Wrong sensor id");
  fail_unless (sensor->data.len == right->data.len, "Wrong sensor data_size" );
  fail_unless (sensor->ts.tv_sec == right->ts.tv_sec, "Wrong sensor timestamp seconds" );
  fail_unless (sensor->ts.tv_usec == right->ts.tv_usec, "Wrong sensor timestamp mseconds" );
}

START_TEST (test_temp_sensor)
{
  char* packets[] = {"{'dev_id': 'temp_1', 'sensor_data': '34.2 C', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '6'}" };
  int degrees[] = { 34, 2 };
  struct sensor sensors[] = { {TEMP, "temp_1", 1, {3231, 7961}, {8, degrees}, NULL, NULL } };

  fail_unless (parse_sensor_update(packets[_i], strlen(packets[_i]), sensor) == 0,
      "Unexpected error in parsing a valid sensor packet");
  compare_sensors(&(sensors[_i]));

  fail_unless (((int*)(sensor->data.blob))[0] == ((int*)(sensors[_i].data.blob))[0], "Wrong sensor value 1");
  fail_unless (((int*)(sensor->data.blob))[1] == ((int*)(sensors[_i].data.blob))[1], "Wrong sensor value 2");
  if(sensor->data.blob) free(sensor->data.blob);
}
END_TEST

START_TEST (test_camera_sensor)
{
  int i = 0;
  char* packets[] = { "{'dev_id': 'camera_1', 'sensor_data': '\\x01', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}",
                      "{'dev_id': 'camera_1', 'sensor_data': '\\\'', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}",
                      "{'dev_id': 'camera_1', 'sensor_data': '\\\\', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}",
                      "{'dev_id': 'camera_1', 'sensor_data': '\\n', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}",
                      "{'dev_id': 'camera_1', 'sensor_data': '\\r', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}",
                      "{'dev_id': 'camera_1', 'sensor_data': '\\t', 'seq_no': '1', 'ts': '3231.7961', 'data_size': '1'}" };
  struct sensor sensors[] = { {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL},
                              {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL},
                              {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL},
                              {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL},
                              {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL},
                              {CAMERA, "camera_1", 1, {3231, 7961}, {1, NULL}, NULL, NULL} };
  for( i = 0; i < 6; i++) {
    sensors[i].data.blob = malloc(sizeof(char));
  }

  *((char*)(sensors[0].data.blob)) = 1;
  *((char*)(sensors[1].data.blob)) = '\'';
  *((char*)(sensors[2].data.blob)) = '\\';
  *((char*)(sensors[3].data.blob)) = '\n';
  *((char*)(sensors[4].data.blob)) = '\r';
  *((char*)(sensors[5].data.blob)) = '\t';

  fail_unless (parse_sensor_update(packets[_i], strlen(packets[_i]), sensor) == 0,
      "Unexpected error in parsing a valid sensor packet");
  compare_sensors(&(sensors[_i]));

  fail_unless (sensor->data.len == sensors[_i].data.len, "Wrong data_size in data");
  fail_unless (((char*)(sensor->data.blob))[0] == ((char*)(sensors[_i].data.blob))[0], "Wrong sensor value");
  if(sensor->data.blob) free(sensor->data.blob);
  for(i = 0; i < 6; i++)
    free(sensors[i].data.blob);
}
END_TEST

START_TEST (test_bug_sensor)
{
  char* packets[] = {"{'dev_id': 'nounderscore'}",
                     NULL};
  fail_unless (parse_sensor_update(packets[0], strlen(packets[0]), sensor) == 0,
      "Error in parsing a sensor packet");
  print_sensor(sensor);
}
END_TEST

Suite *
sensor_parser_suite(void)
{
  Suite *s = suite_create ("Sensor parser");

  TCase *tc_core = tcase_create ("Core");
  TCase *tc_bug = tcase_create ("Bug");

  /* Core test case */
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_loop_test (tc_core, test_temp_sensor, 0, 1);
  tcase_add_loop_test (tc_core, test_camera_sensor, 0, 6);
  suite_add_tcase (s, tc_core);

  /* Bug test case */
  tcase_add_checked_fixture (tc_bug, setup, teardown);
  tcase_add_test (tc_bug, test_bug_sensor);
  suite_add_tcase (s, tc_bug);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = sensor_parser_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
