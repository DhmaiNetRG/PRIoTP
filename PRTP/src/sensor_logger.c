#include "sensor_logger.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

static struct sensor_logger* sensor_logger_list = NULL;
static struct sensor_dump* sensor_dump_list = NULL;

void init_sensor_logger(char* dir)
{
  mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  // chdir(dir);
  if (chdir(dir) != 0) {
    return;
  }
}

struct logger* __new_sensor_logger(const char* id, const char* extension)
{
  struct logger *l;
  FILE* sensor_file;
  char filename[30] = {0};
 
  strcpy(filename, id);
  strcat(filename, extension);

  sensor_file = fopen(filename, "a");
  l = init_logger(sensor_file, NULL, NULL, "");

  return l;
}

struct sensor_logger* __add_sensor_logger(const char* id, const char* extension)
{
  struct sensor_logger* sl;

  sl = xalloc(sizeof(struct sensor_logger));
  sl->logger = __new_sensor_logger(id, extension);
  sl->id = xalloc(strlen(id)+1);
  strcpy(sl->id, id);

  sl->next = sensor_logger_list;
  sensor_logger_list = sl;

  return sl;
}

struct sensor_logger* add_sensor_logger(const char* id)
{
  
  struct sensor_logger* sl = __add_sensor_logger(id, ".log");
  return sl;
}


FILE* __new_sensor_dump(const char* id, const char* extension)
{
  FILE* sensor_file;
  char filename[30] = {0};
 
  strcpy(filename, id);
  strcat(filename, extension);

  sensor_file = fopen(filename, "ab");

  return sensor_file;
}

struct sensor_dump* __add_sensor_dump(const char* id, const char* extension)
{
  struct sensor_dump* sl;

  sl = xalloc(sizeof(struct sensor_dump));
  sl->dump = __new_sensor_dump(id, extension);
  sl->id = xalloc(strlen(id)+1);
  strcpy(sl->id, id);

  sl->next = sensor_dump_list;
  sensor_dump_list = sl;

  return sl;
}

struct sensor_dump* add_sensor_dump(const char* id)
{
  struct sensor_dump* sl = __add_sensor_dump(id, ".data");
  return sl;
}

void remove_sensor_logger(struct sensor_logger* l)
{
  fclose(l->logger->f_print);
  free(l->id);
  shutdown_logger(l->logger);
  free(l);
}

void remove_sensor_dump(struct sensor_dump* l)
{
  fclose(l->dump);
  free(l->id);
  free(l);
}

void shutdown_sensor_logger()
{
  struct sensor_logger* node = sensor_logger_list;
  struct sensor_dump* d_node = sensor_dump_list;

  while( node != NULL ) {
    sensor_logger_list = node->next;
    remove_sensor_logger(node);
    node = sensor_logger_list;
  }

  while( d_node != NULL ) {
    sensor_dump_list = d_node->next;
    remove_sensor_dump(d_node);
    d_node = sensor_dump_list;
  }
}

void sensor_dump(const char* id, const struct void_data* data)
{
  struct sensor_dump* node;

  for( node = sensor_dump_list; node != NULL; node = node->next )
  {
    if( strcmp( id, node->id ) == 0 ) break;
  }
  if( node ) {
    if( data->blob ) {
      fwrite((char*)(data->blob), 1, data->len, node->dump);
      /* log_print(node->logger, (char*)data + sizeof(int));*/
    }
  }
}

void sensor_log(const char* id, int seq_no, enum SENSOR_TYPE type, const struct void_data* data)
{
  struct sensor_logger* node;
  struct timeval ts;

  for( node = sensor_logger_list; node != NULL; node = node->next )
  {
    if( strcmp( id, node->id ) == 0 ) break;
  }
  if( node ) {
    gettimeofday(&ts, 0);

    log_print(node->logger, "%u.%02u\t%u\t", ts.tv_sec, ts.tv_usec/10000, seq_no);
    print_sensor_data(node->logger, type, data);
    log_print(node->logger, "\n");
  }
}
