#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sensor_types.h"
#include "utils.h"

static struct logger* l = NULL;

void init_sensor_types()
{
  l = init_logger(stdout, stderr, stderr, "SensorTypes");
}

void shutdown_sensor_types()
{
  shutdown_logger(l);
}

static size_t sensor_sizes[] = {
  0,
  2*sizeof(int),
  sizeof(char),
  2*sizeof(double),
  0
};

enum SENSOR_TYPE parse_sensor_type(const char* buf)
{
  char type[ID_SIZE] = {0}; /* Will put sensor type here */
  const char *cur;
  char *type_cur = type;
  enum SENSOR_TYPE ret = UNKNOWN;

  for(cur = buf; (*cur != 0) && ((type_cur - type) < ID_SIZE); cur++)
  {
    /* fixed. there was a bug here when the string doesn't contain '_' */
    if( *cur == '_' ) {
      *type_cur = 0;
      if     ( !strcmp(type, "temp" ) )   ret = TEMP;
      else if( !strcmp(type, "device" ) ) ret = DEVICE;
      else if( !strcmp(type, "gps" ) )    ret = GPS;
      else if( !strcmp(type, "camera" ) ) ret = CAMERA;
      break;
    }
    else {
      *type_cur = *cur;
      ++type_cur;
    }
  }
  if( *cur == 0 ) {
    /*log_error(l, "Warning: the device id does not contain a valid sensor type.\n");*/
  }
  if( (type_cur - type) == ID_SIZE ) {
    /*log_error(l, "Warning: the device id too long.\n");*/
  }

  return ret;
}

void sensor_type_to_string(enum SENSOR_TYPE type, char* res)
{
  switch(type) {
  case TEMP:
    sprintf(res, "Temperature");
    break;
  case DEVICE:
    sprintf(res, "Device");
    break;
  case GPS:
    sprintf(res, "GPS");
    break;
  case CAMERA:
    sprintf(res, "Camera");
    break;
  case UNKNOWN:
    sprintf(res, "Unknown");
    break;
  }
}

int sensor_alloc_data(enum SENSOR_TYPE type, struct void_data* data)
{
  if( data->len == 0 )
    data->len = sensor_sizes[type] ? sensor_sizes[type] : 5000;
  data->blob = xalloc(data->len);
  if( data->len == 5000 ) data->len = 0;

  if(!(data->blob)) return -1;
  return data->len;
}


/* Camera and unknown data has int in the beginning as data size
 * It was a bad idea, now data is a structure with an integer field */
// int sensor_copy_data(enum SENSOR_TYPE type, struct void_data* to, const struct void_data* from)
// {
//   to->len = from->len;
//   to->sensor_type = from->sensor_type;
//   to->sid = from->sid;

//   log_debug( l, "Tracing copy data. %s %d\n", to->sid , to->sensor_type );

//   if( sensor_sizes[type] ) {
//     memcpy( to->blob, from->blob, sensor_sizes[type] );
//     if( from->len != sensor_sizes[type] ) to->len = sensor_sizes[type];
//     return 0;
//   }
//   else {
//     memcpy( to->blob, from->blob, from->len );
//     return 0;
//   }

//   return -1;
// }
int sensor_copy_data(enum SENSOR_TYPE type, struct void_data* to, const struct void_data* from)
{
  to->len = from->len;
  // Remove these lines - sensor_type and sid are NOT part of void_data
  // to->sensor_type = from->sensor_type;  
  // to->sid = from->sid;                   

  if( sensor_sizes[type] ) {
    memcpy( to->blob, from->blob, sensor_sizes[type] );
    if( from->len != sensor_sizes[type] ) to->len = sensor_sizes[type];
    return 0;
  }
  else {
    memcpy( to->blob, from->blob, from->len );
    return 0;
  }

  return -1;
}

int sensor_set_data(enum SENSOR_TYPE type, struct void_data* to, const struct void_data* value)
{
  int ret = -1;

  to->len = sensor_sizes[type] ? sensor_sizes[type] : value->len;

  if( (sensor_alloc_data(type, to) != -1) )
    ret = sensor_copy_data(type, to, value);

  return ret;
}

/*int sensor_set_data_range(enum SENSOR_TYPE type, void** to, const void* value, uint32_t offset, uint32_t length)
{
  *to = xalloc( length + sizeof(uint32_t) );
  memcpy( *to, &length, sizeof(uint32_t) );
  memcpy( *to + sizeof(uint32_t), value + offset, length );
  return 0;
}
*/
int sensor_copy_offset_data(enum SENSOR_TYPE type, int offset, struct void_data* data,
                            const struct void_data* from)
{
  int ret = -1;

  switch(type) {
  case TEMP:
    if( (offset >= 0) && (offset <= 1) ) {
      ((int*)(data->blob))[offset] = *((int*)(from->blob));
      ret = 0;
    }
    break;
  case DEVICE:
    if( offset == 0 ) {
      ((char*)(data->blob))[offset] = *((char*)(from->blob));
      ret = 0;
    }
    break;
  case GPS:
    if( (offset >= 0) && (offset <= 1) ) {
      ((double*)(data->blob))[offset] = *((double*)(from->blob));
      ret = 0;
    }
    break;
  case CAMERA:
    data->len = from->len;
    memcpy( &(((char*)(data->blob))[offset]), from->blob, from->len );
    break;
  default:
    break;
  }

  return ret;
}

void print_sensor_data(struct logger* log, enum SENSOR_TYPE type, const struct void_data* data)
{
  switch(type) {
  case TEMP:
    print_temperature_data(log, data);
    break;
  case DEVICE:
    print_device_data(log, data);
    break;
  case GPS:
    print_gps_data(log, data);
    break;
  case CAMERA:
    print_camera_data(log, data);
    break;
  case UNKNOWN:
    print_unknown_data(log, data);
    break;
  }
}

void print_device_data(struct logger* log, const struct void_data* data)
{
  if( *((char*)(data->blob)) == 1 ) log_print(log, "ON");
  else if( *((char*)(data->blob)) == 0 ) log_print(log, "OFF");
  else log_print(log, "Unknown");
}

void print_temperature_data(struct logger* log, const struct void_data* data)
{
  int* degrees = (int*)(data->blob);
  log_print(log, "%d.%d C", degrees[0], degrees[1]);
}

/* Print data size here, used for logging */
void print_camera_data(struct logger* log, const struct void_data* data)
{
  if( data->blob ) log_print(log, "%u", data->len);
  else log_print(log, "Empty camera buffer");
}

void print_gps_data(struct logger* log, const struct void_data* data)
{
  double* coordinates = (double*)(data->blob);
  log_print(log, "[%f, %f]", coordinates[0], coordinates[1]);
}

void print_unknown_data(struct logger* log, const struct void_data* data)
{
  log_print(log, "unknown data to print");
}
