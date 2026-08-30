#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __APPLE__
#include "apple_endian.h"
#else
#include <endian.h>
#endif

#include "bson_parser.h"
#include "bson_msg.h"
#include "logger.h"

#define INITIAL_BUFFER_SIZE 2

static struct logger* l = NULL;

void init_bson_parser()
{
  l = init_logger(stdout, stderr, stderr, "BSON Parser");
  init_bson_msg();
}

void shutdown_bson_parser()
{
  shutdown_logger(l);
  shutdown_bson_msg();
}

int parse_bson_byte(char* buf, uint32_t* bytes_left, char* result)
{
    int field_size = 1;
    if( *bytes_left < field_size ) return -1;
    *result = buf[0];
    *bytes_left = *bytes_left - field_size;
    return field_size;
}

int parse_bson_int(char* buf, uint32_t* bytes_left, int32_t* result)
{
    int field_size = 4;
    if( *bytes_left < field_size ) return -1;
    *result = le32toh(*(int32_t*)buf);
    *bytes_left = *bytes_left - field_size;
    return field_size;
}

/* Returns number of bytes read including the terminating null
 */
int parse_bson_cstring(char* buf, uint32_t *bytes_left, char** result)
{
    char* rbuf;
    char c;
    uint32_t cnt;
    uint32_t buf_size = INITIAL_BUFFER_SIZE;
    if( *bytes_left == 0 ) return -1;
    rbuf = calloc(buf_size, sizeof(char));
    cnt = 0;
    do {
        c = buf[cnt];
        if (cnt == buf_size) {
            void* tmp = NULL;
            buf_size = buf_size * 2;
            tmp = realloc(rbuf, buf_size);
            if (tmp == NULL) {
                free(rbuf);
                return -1;
            }
            rbuf = tmp;
        }
        rbuf[cnt] = c;
        cnt++;
        (*bytes_left)--;
    } while( (c != '\0') && (*bytes_left > 0) );
    if( *bytes_left == 0 ) {
      free(rbuf);
      return -1;
    }
    *result = rbuf;
    return cnt;
}

int parse_bson_double(char* buf, uint32_t* bytes_left, double* result)
{
    int field_size = 8;
    if( *bytes_left < field_size ) return -1;
    *result = (double)le64toh(*(double*)buf);
    *bytes_left = *bytes_left - field_size;
    return field_size;
}

int parse_bson_boolean(char* buf, uint32_t* bytes_left, bool* result)
{
  int field_size = 1;
  if( *bytes_left < field_size ) return -1;
  *bytes_left = *bytes_left - field_size;
  *result = (buf[0] == 0) ? false:true;
  return field_size;
}

int parse_bson_binary(char* buf, uint32_t* bytes_left)
{
  int field_size = -1;

  /* one byte for subtype, 4 bytes for the data_size */
  field_size = *(int*)buf + 5;
  
  if( *bytes_left < field_size ) return -1;
  *bytes_left = *bytes_left - field_size;
  return field_size;
}

int __parse_bson_document(char* buf, uint32_t* bytes_left, struct PRTP_packet** result, char* parent);
int parse_bson_document(char* buf, size_t len, struct PRTP_packet** result)
{
  uint32_t bytes_left = len;
  struct PRTP_packet* tmp = NULL;
  int ret = -1;


  if( !result ) result = &tmp;
  if( *result != NULL ) {
    log_debug(l, "BSON parser warning: possibly overwriting PRTP_packet\n");
    *result = NULL;
  }
  ret = __parse_bson_document(buf, &bytes_left, result, NULL);
  if( tmp ) free_iotmsg(tmp);
  if( ret == -1 ) {
    if( *result ) free_iotmsg( *result );
  }
  return ret;
}

int __parse_bson_document(char* buf, uint32_t* bytes_left, struct PRTP_packet** result, char* parent)
{
    /* int32_t error; */
    int res;
    uint32_t initial_bytes_left;
    int32_t doc_len;
    char* key = NULL;
    char value_type = -1;
    char* value_str;
    int32_t value_int32;
    int32_t value_len;
    double value_double;
    bool value_bool;
    /* Was used to hold the added sid into sids of PRTP_packet
     * Now the approach is to assume that sids head in PRTP_packet is pointing to the last added sid */
    /* struct iotmsg_node* node; */

      log_debug(l, "Parsing client packet\n");

    initial_bytes_left = *bytes_left;
    if( (res = parse_bson_int(buf, bytes_left, &doc_len)) != -1 )
      buf += res;
    else {
      log_debug(l, "BSON parser: cannot parse document length.\n");
      return -1;
    }
    log_debug(l, "Document length: %d bytes\n", doc_len);
    if( doc_len <= 0 ) {
      log_debug(l, "BSON parser: zero document length.\n");
      return -1;
    }

    while (*bytes_left > 0) {
        if( (res = parse_bson_byte(buf, bytes_left, &value_type)) == -1 ) {
          log_debug(l, "BSON parser: unexpected end of input.\n");
          return -1;
        }
        buf += res;
        if (value_type == 0) {
          log_debug(l, "BSON parser: return. %d length document has %d bytes.\n",
              doc_len, (initial_bytes_left - *bytes_left));
          return (initial_bytes_left - *bytes_left);
        }
        if( (res = parse_bson_cstring(buf, bytes_left, &key)) == -1 ) {
          log_debug(l, "BSON parser: cannot parse the key.\n");
          return -1;
        }
        buf += res;

        log_debug(l, "BSON parser: type: '%d' key: '%s'\n", value_type, key);
        switch(value_type) {
            case BSON_DOUBLE:
                log_debug(l, "BSON parser: double\n");
                if( (res = parse_bson_double(buf, bytes_left, &value_double)) == -1 ) break;
                buf += res;
                log_debug(l, "%f\n", value_double);
                res = bson_iotmsg_double(result, parent, key, value_double);
                break;
            case BSON_STRING:
                log_debug(l, "BSON parser: string\n");
                if( (res = parse_bson_int(buf, bytes_left, &value_len)) == -1 ) break;
                buf += res;
                if( (res = parse_bson_cstring(buf, bytes_left, &value_str)) == -1 ) break;
                buf += res;

                log_debug(l, "BSON parser: %d '%s'\n", value_len, value_str);
                res = bson_iotmsg_string(result, parent, key, value_str);
                free(value_str);
                break;
            /* Arrays are handled as documents where each key is an empty string */
            case BSON_ARRAY:
                log_debug(l, "BSON parser: array\n");
                if( (res = __parse_bson_document(buf, bytes_left, result, key)) == -1 ) break;
                buf += res;
                break;
            case BSON_DOCUMENT:
                log_debug(l, "BSON parser: object\n");
                if( (res = __parse_bson_document(buf, bytes_left, result, key)) == -1 ) break;
                buf += res;
                break;
            case BSON_BINARY:
                log_debug(l, "BSON parser: binary\n");
                if( (res = parse_bson_binary(buf, bytes_left)) == -1 ) break;
                buf += res;
                res = bson_iotmsg_binary(result, parent, key, buf-res);
                break;
            case BSON_BOOLEAN:
                if( (res = parse_bson_boolean(buf, bytes_left, &value_bool)) == -1 ) break;
                buf += res;
                if( value_bool ) log_debug(l, "BSON parser: boolean true\n");
                else log_debug(l, "BSON parser: boolean false\n");
                res = bson_iotmsg_boolean(result, parent, key, value_bool);
                break;
            case BSON_NULL:
                log_debug(l, "BSON parser: null\n");
                res = -1;
                break;
            case BSON_INT32:
                log_debug(l, "BSON parser: int32:\n");
                if( (res = parse_bson_int(buf, bytes_left, &value_int32)) == -1 ) break;
                buf += res;
                log_debug(l, "%d\n", value_int32);
                res = bson_iotmsg_int32(result, parent, key, value_int32);
                break;
            case BSON_TIMESTAMP:
                log_debug(l, "BSON parser: timestamp\n");
                res = -1;
                break;
            case BSON_INT64:
                log_debug(l, "BSON parser: int64\n");
                res = -1;
                break;
            default:
                log_debug(l, "BSON parser: unknown type: %d\n", value_type);
                res = -1;
                break;
        }
        free(key);
        if( res == -1 ) return -1;
    }
    log_debug(l, "BSON parser: unexpected end of input.\n");
    return -1;
}

int write_bson_int(char* buf, const char* key, int32_t value)
{
  size_t key_len = strlen(key);
  if( buf == NULL ) return key_len + 6;
  
  buf[0] = BSON_INT32;
  strcpy(&(buf[1]), key);

  *(int32_t*)(buf + key_len + 2) = htole32(value);

  return key_len + 6;
}

int write_bson_double(char* buf, const char* key, double value)
{
  size_t key_len = strlen(key);
  if( buf == NULL ) return key_len + 10;
  
  buf[0] = BSON_DOUBLE;
  strcpy(&(buf[1]), key);

  *(double*)(buf + key_len + 2) = htole64(value);

  return key_len + 10;
}

int write_bson_doclen(char* buf, uint32_t value)
{
  if( buf == NULL ) return 4;
  *(int32_t*)buf = htole32(value);
  return 4;
}

int write_bson_array(char* buf, const char* key)
{
  size_t key_len = strlen(key);
  if( buf == NULL ) return key_len + 2;
  
  buf[0] = BSON_ARRAY;
  strcpy(&(buf[1]), key);

  return key_len + 2;
}

int write_bson_object(char* buf, const char* key)
{
  size_t key_len = strlen(key);
  if( buf == NULL ) return key_len + 2;
  
  buf[0] = BSON_DOCUMENT;
  strcpy(&(buf[1]), key);

  return key_len + 2;
}

int write_bson_string(char* buf, const char* key, const char* value)
{
  size_t key_len = strlen(key);
  uint32_t value_len = strlen(value);
  if( buf == NULL ) return key_len + value_len + 7;
  
  buf[0] = BSON_STRING;
  strcpy(&(buf[1]), key);

  *(uint32_t*)(buf + key_len + 2) = htole32(value_len);
  strcpy(&(buf[key_len + 6]), value);
  return key_len + value_len + 7;
}

int write_bson_boolean(char* buf, const char* key, bool value)
{
  size_t key_len = strlen(key);
  if( buf == NULL ) return key_len + 3;
  
  buf[0] = BSON_BOOLEAN;
  strcpy(&(buf[1]), key);

  buf[key_len + 2] = value ? 1:0;
  return key_len + 3;
}

/* Will write 0 (generic) as a subtype. */
int write_bson_binary(char* buf, const char* key, const struct void_data* value)
{
  size_t key_len = strlen(key);
  /* type,key,'0',int32,'0',data */
  if( buf == NULL ) return key_len + 7 + value->len;
  
  buf[0] = BSON_BINARY;
  strcpy(&(buf[1]), key);

  *(int32_t*)(buf + key_len + 2) = htole32(value->len);
  buf[key_len + 6] = 0; /* generic subtype */
  memcpy(buf + key_len + 7, value->blob, value->len);
  return key_len + 7 + value->len;
}
