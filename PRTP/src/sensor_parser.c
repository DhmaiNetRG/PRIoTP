#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sensor_parser.h"
#include "logger.h"
#include "utils.h"

#define KEY_SIZE 20 /* For keys in incoming sensor packets */

static struct logger* l = NULL;

void init_sensor_parser()
{
  l = init_logger(stdout, NULL, stderr, "Sensor Parser");
}

void shutdown_sensor_parser()
{
  shutdown_logger(l);
}


void print_sensor(const struct sensor* sensor)
{
  char buf[18] = {0};
  sensor_type_to_string(sensor->type, buf);
  log_print(l, "%s sensor %s, seq_no %u, %u bytes at %lu.%02lu is ",
         buf, sensor->id,
         sensor->seq_no, sensor->data.len,
         sensor->ts.tv_sec, sensor->ts.tv_usec);
  sensor->print_data(l, &(sensor->data));
  printf("\n");
}

/* Parse a key name. Returns -1 if a key is unknown
 * Return value will be used in parse_data() to actually assign value to the key */
int parse_key(const char* buf)
{
  int ret = -1;

  if( strlen(buf) >= KEY_SIZE ) return ret;

  if( !strcmp(buf, "dev_id") )      ret = 1;
  if( !strcmp(buf, "sensor_data") ) ret = 2;
  if( !strcmp(buf, "seq_no") )      ret = 3;
  if( !strcmp(buf, "ts") )          ret = 4;
  if( !strcmp(buf, "data_size") )   ret = 5;

  return ret;
}

/*
 * Sensor-type specific data parsers
 * Will allocate memory for sensor->data
 * Caller must free
 */
int parse_device_data(struct sensor* sensor, const char* buf)
{
  char state = 0;
  struct void_data data;
  if( !strcmp(buf, "ON") ) state = 1;
  else if( !strcmp(buf, "OFF") ) state = 0;
  else {
    log_error(l, "Unexpected device sensor state\n");
    return -1;
  }

  data.len = sizeof(char);
  data.blob = &state;
  data.sid = sensor->id;
  data.sensor_type = sensor->type;
  sensor_set_data(sensor->type, &(sensor->data), &data);
  return 0;
}

int parse_temperature_data(struct sensor* sensor, const char* buf)
{
  int degrees[2];
  struct void_data data;
  if( sscanf(buf, "%d.%d C", &degrees[0], &degrees[1]) < 2 ) {
    log_error(l, "Error parsing temperature data.\n");
    return -1;
  }

  data.len = 2*sizeof(int);
  data.blob = degrees;
  data.sid = sensor->id;
  data.sensor_type = sensor->type;
  sensor_set_data(sensor->type, &(sensor->data), &data);
  return 0;
}

char hex2char(char* hex)
{
  char res = 0;

  if( (hex[0] >= '0') && (hex[0] <= '9') ) res = (hex[0]-'0')*16;
  else if ( (hex[0] >= 'A') && (hex[0] <= 'F') ) res = (10+hex[0]-'A')*16;
  else if ( (hex[0] >= 'a') && (hex[0] <= 'f') ) res = (10+hex[0]-'a')*16;

  if( (hex[1] >= '0') && (hex[1] <= '9') ) res += (hex[1]-'0');
  else if ( (hex[1] >= 'A') && (hex[1] <= 'F') ) res += (10+hex[1]-'A');
  else if ( (hex[1] >= 'a') && (hex[1] <= 'f') ) res += (10+hex[1]-'a');

  return res;
}

int parse_camera_data(struct sensor* sensor, const char* buf)
{
  char temp[BUF_SIZE + 1] = {0};
  const char *cur;
  char *temp_cur = temp; 
  unsigned int backslash = 0;
  unsigned int hex = 0;
  char temp_hex[2];
  int data_size = 0;

  for( cur = buf; *cur != 0; cur++ ) {
    /* Read two symbols and convert from hex to char */
    if( hex > 0 ) {
      temp_hex[hex - 1] = *cur;
      hex++;
      if( hex == 3 ) {
        *temp_cur = hex2char(temp_hex);
        temp_cur++;
        hex = 0;
      }
      continue;
    }
    if( *cur == 'x' && backslash ) {
      backslash = 0;
      hex = 1;
      continue;
    }
    if( backslash ) {
      if( *cur == 'n' ) {
        *temp_cur = '\n'; temp_cur++; backslash = 0; continue; }
      if( *cur == 'r' ) {
        *temp_cur = '\r'; temp_cur++; backslash = 0; continue; }
      if( *cur == 't' ) {
        *temp_cur = '\t'; temp_cur++; backslash = 0; continue; }
      if( *cur == '\\' ) {
        *temp_cur = '\\'; temp_cur++; backslash = 0; continue; }
    }
    if( *cur == '\\' ) {
      backslash = 1;
      continue;
    }
    backslash = 0;
    *temp_cur = *cur;
    temp_cur++;
  }

  data_size = temp_cur - temp;

  if( sensor->data.len ) {
    if( sensor->data.len != data_size ) {
      log_error(l, "Warning: data_size in the packet does not match the actual size of data.  %d != %d.\n",
                sensor->data.len, data_size);
    }
  }
  
  sensor->data.len = data_size;
  sensor->data.blob = xalloc(data_size);
  memcpy((char*)sensor->data.blob, temp, data_size);

  return 0;
}

int parse_gps_data(struct sensor* sensor, const char* buf)
{
  double coordinates[2];
  struct void_data data;

  if( sscanf(buf, "[%lf, %lf]", &coordinates[0], &coordinates[1]) < 2 ) {
    log_error(l, "Error parsing GPS data.\n");
    return -1;
  }

  data.blob = coordinates;
  data.len = 2*sizeof(double);
  data.sid = sensor->id;
  data.sensor_type = sensor->type;
  sensor_set_data(sensor->type, &(sensor->data), &data);
  return 0;
}

int parse_unknown_data(struct sensor* sensor, const char* buf)
{
  return -1;
}

void set_parse_data(struct sensor* sensor)
{
  switch(sensor->type) {
  case TEMP:
    sensor->parse_data = parse_temperature_data;
    sensor->print_data = print_temperature_data;
    break;
  case DEVICE:
    sensor->parse_data = parse_device_data;
    sensor->print_data = print_device_data;
    break;
  case GPS:
    sensor->parse_data = parse_gps_data;
    sensor->print_data = print_gps_data;
    break;
  case CAMERA:
    sensor->parse_data = parse_camera_data;
    sensor->print_data = print_camera_data;
    break;
  case UNKNOWN:
    sensor->parse_data = parse_unknown_data;
    sensor->print_data = print_unknown_data;
    break;
  }
}

/*
 * Parse a value. Returns -1 if parsing fails
 * Caller must take care of res->data. If it's not NULL, it was allocated
 * and should be free'd.
 */
int parse_sensor_data(const char* buf, int key, struct sensor* res)
{
  int ret = 0;
  long unsigned int sec, usec; /* For timestamp */

  switch(key) {
  case 1:
    res->type = parse_sensor_type(buf);
    strncpy(res->id, buf, ID_SIZE);
    res->id[ID_SIZE] = 0;
    set_parse_data(res);
    break;
  case 2:
    if( res->parse_data != NULL ) ret = res->parse_data(res, buf);
    break;
  case 3:
    if( (res->seq_no = atoi(buf)) == 0 ) ret = -1;
    break;
  case 4:
    if( sscanf(buf, "%lu.%lu", &sec, &usec) < 2 ) ret = -1;
    else {
      res->ts.tv_sec = sec;
      res->ts.tv_usec = usec;
    }
    break;
  case 5:
/*    if( res->data.len ) {
      if( res->data.len != atoi(buf) ) */
        /* Or we encountered data_size field again, but this should be the task of validator */
/*        log_error(l, "Warning: data_size in the packet does not match the actual size of data.  %d != %d.\n",
                res->data.len, atoi(buf));
    }
    res->data.len = atoi(buf);
*/
    break;
  default:
    ret = -1;
  }

  return ret;
}

int parse_sensor_update(const char* buf, ssize_t len, struct sensor* res)
{
  const char* cur = buf;
  char temp[BUF_SIZE + 1] = {0};
  int key = 0;
  char* temp_cur = temp;
  /* Parser state:
   * 0 - outside of the packet, search for {
   * 1 - inside the packet, outside key/value pair, search for '\'' or '"'
   * 2 - inside the string, inside key, search for '\'' or '"'
   * 3 - outside the string, between key and value, search for ':'
   * 4 - outside the string, between key and value, search for '\'' or '"'
   * 5 - inside the string, inside value, search for '\'' or '"'
   * 6 - outside the string, search for ',' or }
   * 7 - outside the packet, should not have anything in input (xxx not checked)
  */
  unsigned int state = 0;
  char se = '\0'; /* String end, '\'' or '"' */
  unsigned int backslash = 0; /* Backslashing ' or " or "\" */

  memset(res, 0, sizeof(struct sensor));
  for(cur = buf; cur < buf + len; cur++) {
    if( state == 0 ) {
      if( *cur == '{' ) {
        state = 1;
        continue;
      }
      log_error(l, "Packet must start with '{'.\n");
      break;
    }
    if( (state == 1) || (state == 4) ) {
      if( *cur == ' ' ) continue;
      if( (*cur == '\'') || (*cur == '\"') ) {
        state++;
        se = *cur;
        temp_cur = temp;
        continue;
      }
      log_error(l, "Non-space character while search for a start of a string.\n");
      log_error(l, buf);
      break;
    }
    if( state == 3 ) {
      if( *cur == ' ' ) continue;
      if( *cur == ':' ) {
        state++;
        continue;
      }
      log_error(l, "Non-space character while search for a key-value delimiter.\n");
      log_error(l, buf);
      break;
    }
    if( (state == 2) || (state == 5) ) {
      /* '\'' or '"' might be backslashed when inside string
       * And backslash might be backslashed itself */
      if( (*cur == se) && !backslash ) {
        *temp_cur = 0;
        if( state == 2 )
          if( (key = parse_key(temp)) == -1 ) break;
        if( state == 5 )
          if( parse_sensor_data(temp, key, res) == -1 ) break;
        log_debug(l, "Got in state %u string %s\n", state, temp);
        state++;
        backslash = 0;
        continue;
      }
      *temp_cur = *cur;
      ++temp_cur;
      if( (*cur == '\\') ) {
        if( backslash ) backslash = 0;
        else backslash = 1;
      }
      else backslash = 0;
    }
    if( state == 6 ) {
      if( *cur == ' ' ) continue;
      if( *cur == ',' ) {
        state = 1;
        continue;
      }
      if( *cur == '}' ) {
        state = 7;
        break;
      }
      log_error(l, "Non-space character while search for a pairs delimiter or the end of a message.\n");
      log_error(l, buf);
      break;
    }
  }

  /* printf("State %u\n", state);
   * printf("%s\n", buf);
   * printf("%s\n", cur);
   */
  if( state != 7 ) {
    if(res->data.blob) free(res->data.blob);
    return -1;
  }
  else return 0;
}
