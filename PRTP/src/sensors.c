#include "sensors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "utils.h"

#include "sensor_parser.h"
#include "sensor_logger.h"

static struct sensor_node* sensors_list = NULL;
static struct logger* l = NULL;
void clear_sensor_list();

void init_sensors()
{
  l = init_logger(stdout, stderr, stderr, "Sensors");
}

void shutdown_sensors()
{
  clear_sensor_list();
  shutdown_logger(l);
}

/* Create PRTP_packet->sids list from sensors_list */
void write_sensor_list(struct PRTP_packet* msg)
{
  struct sensor_node* s_node;
  struct iotmsg_node* m_node;

  msg->data.blob = NULL;
  for( s_node = sensors_list; s_node != NULL; s_node = s_node->next )
  {
    m_node = xalloc(sizeof(struct iotmsg_list_node));
    m_node->id = xalloc(strlen(s_node->id) + 1);
    strcpy(m_node->id, s_node->id);
    m_node->next = msg->data.blob;
    msg->data.blob = m_node;
  }
}

void clear_sensor_list()
{
  struct sensor_node* node = sensors_list, *tmp = NULL;

  while( node != NULL ) {
    tmp = node->next;
    free(node->id);
    free(node->data.blob);
    free(node);
    node = tmp;
  }
}

/* Print all registered sensors
 * Currently only ids */
void print_sensors()
{
  struct sensor_node* node;
  for( node = sensors_list; node != NULL; node = node->next )
    log_print( l, "Sensor %s\n", node->id);
}

struct sensor_node* get_sensor(const char* id)
{
  struct sensor_node* node;
  if( id == NULL ) return NULL;
  for( node = sensors_list; node != NULL; node = node->next )
    if( strcmp( node->id, id ) == 0 ) return node;
  return NULL;
}

struct sensor_node* add_sensor(const char* id, enum SENSOR_TYPE type)
{
  struct sensor_node* node;

  node = xalloc(sizeof(struct sensor_node));

  node->id = xalloc(strlen(id) + 1);
  strcpy(node->id, id);
  node->type = type;

  sensor_alloc_data(type, &(node->data));

  node->next = sensors_list;
  sensors_list = node;

  log_debug(l, "\n\n\nTesting Mehraj id %s\n", id);

  add_sensor_logger(id);
  if( type == CAMERA ) add_sensor_dump(id);

  return node;
}

void populate_sensor_list(const char* filename)
{
  FILE* sensor_file = NULL;
  char str[100];
  char* ch;

  sensor_file = fopen(filename, "r");
  if( !sensor_file )
  {
    log_error( l, "Couldn't open sensor list file %s.\n", filename );
    return;
  }

  while( !feof( sensor_file ) ) {
    ch = fgets(str, 100, sensor_file);
    if( !ch ) break;
    ch = strchr( str, '\n' );
    if( ch ) *ch = '\0'; 
    add_sensor(str, parse_sensor_type( str ));
  }
}

int set_sensor_data(struct sensor_node* node, struct void_data* data)
{
  int ret = -1;
  ret = sensor_copy_data(node->type, &(node->data), data);
  return ret;
}

/* Read a packet from sensors */
int read_sensor(int sd, struct sensor_node** ret_node)
{
  /* Adding terminating NULL-character for printing */
  char buf[BUF_SIZE + 1];
  int ret = -1;
  ssize_t recvlen;
  struct sensor_node* node;
  struct sensor parsed_sensor = {0};

  recvlen = recv(sd, buf, BUF_SIZE, 0);      //data from socket is saved into buffer
  if (recvlen == 0) {
      log_error(l, "Connection is closed. \n");
      return ret;
   }
  else if (recvlen == -1) {
      log_error(l, "Recvfrom failed. \n");
      return ret;
  }
  log_debug(l, "Received %u bytes from sensor\n", recvlen);
  buf[recvlen] = '\0';

  if( parse_sensor_update(buf, recvlen, &parsed_sensor) != -1)  //parsed sensor is where the sensor data is stored into
  {
    ret = 0;
    print_sensor(&parsed_sensor);

    for( node = sensors_list; node != NULL; node = node->next )  //first it checks whether the sensor is added
    {
      if( strcmp(node->id, parsed_sensor.id) == 0 ) break;
    }
    /* New sensor, add to the list */
    if( node == NULL ) {
      node = add_sensor(parsed_sensor.id, parsed_sensor.type);
      ret = 1;
    }
    /* update the sensor */
    set_sensor_data(node, &(parsed_sensor.data));
    node->seq_no = parsed_sensor.seq_no;
    if(parsed_sensor.data.blob) free(parsed_sensor.data.blob);
    *ret_node = node;
  }

  print_sensors();
  return ret;
}

int sensor_socket(const char* hostname, in_port_t port)
{
  int sd;

  if( (sd = init_socket(hostname, port, true, NULL)) == -1 ){
    log_error(l, "Sensor socket init failed.\n");
    return -1;
  }
  
  return sd;
}
