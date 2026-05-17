#ifndef SENSOR_LOGGER_H
#define SENSOR_LOGGER_H

#include "logger.h"
#include "sensor_types.h"
#include <stdbool.h>

struct sensor_logger
{
  char* id;
  struct logger* logger;
  struct sensor_logger* next;
};

struct sensor_dump
{
  char* id;
  FILE* dump;
  struct sensor_dump* next;
};

struct sensor_dump* add_sensor_dump(const char* id);
struct sensor_logger* add_sensor_logger(const char* id);

void sensor_log(const char* id, int seq_no, enum SENSOR_TYPE type, const struct void_data* data);
void sensor_dump(const char* id, const struct void_data* data);

void init_sensor_logger();
void shutdown_sensor_logger();

#endif

