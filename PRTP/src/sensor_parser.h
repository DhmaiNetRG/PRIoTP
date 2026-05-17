#ifndef SENSOR_PARSER_H
#define SENSOR_PARSER_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include "logger.h"
#include "void_data.h"

#include "sensor_types.h"
/*
 * Should be enough to capture camera data
 *   15 fps at 50-200 Kbps leads to 1666 bytes max
 *   Encoded with "\x00" gives roughly 5000 bytes per datagram for 200 Kbps
 *   Additional room for dev_id, seq_no etc.
 */
#define BUF_SIZE 5500

/* To get parsed data from the packet */
struct sensor
{
  enum SENSOR_TYPE type;
  char id[ID_SIZE + 1];
  uint32_t seq_no;
  struct timeval ts;
  struct void_data data;
  int (*parse_data)(struct sensor*, const char*);
  void (*print_data)(struct logger* log, const struct void_data* data);
};


/*
 * Parse raw JSON-ish packets from a sensor 
 * buf must be NULL-terminated string (for debug printing)
 * len must be not more than BUF_SIZE
 * parsed sensor packet returned as res
 */
int parse_sensor_update(const char* buf, ssize_t len, struct sensor* res);

/* Prints struct sensor */
void print_sensor(const struct sensor* sensor);

void init_sensor_parser();
void shutdown_sensor_parser();

#endif
