#ifndef SENSORS_H
#define SENSORS_H

#include <netinet/in.h>
#include "sensor_types.h"
#include "messages.h"
#include "void_data.h"

struct sensor_node {
  char* id;
  struct sensor_node* next;
  enum SENSOR_TYPE type;
  int seq_no;
  struct void_data data;
};

int sensor_socket(const char* hostname, in_port_t port);

/* Get a packet from the socket, parse it and return result in sensor */
int read_sensor(int sd, struct sensor_node** sensor);

/* Create sids structure of the message with all registered sensors */
void write_sensor_list(struct PRTP_packet* msg);

/* Get one sensor by its id */
struct sensor_node* get_sensor(const char* id);

/* Tries to open the file and read a sensor list from it */
void populate_sensor_list(const char* filename);

void init_sensors();
void shutdown_sensors();

#endif
