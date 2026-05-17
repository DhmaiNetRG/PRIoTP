#ifndef BSON_PARSER_H

#define BSON_PARSER_H

#include <stdbool.h>
#include "messages.h"

enum BSON_TYPE {
    BSON_UNKNOWN__,
    BSON_DOUBLE,
    BSON_STRING,
    BSON_DOCUMENT,
    BSON_ARRAY,
    BSON_BINARY,
    BSON_UNKNOWN_6,
    BSON_OBJECTID,
    BSON_BOOLEAN,
    BSON_UTCDATETIME,
    BSON_NULL,
    BSON_REGEXP,
    BSON_UNKNOWN_12,
    BSON_JSCODE,
    BSON_UNKNOWN_14,
    BSON_JSCODE_W_S,
    BSON_INT32,
    BSON_TIMESTAMP,
    BSON_INT64
};

/* int parse_bson_array(char* buf, uint32_t* bytes_read, struct PRTP_packet* result); */
int parse_bson_document(char* buf, size_t len, struct PRTP_packet** result);

/* Allow access to writers for bson_msg to be able to serialize PRTP_packet */
int write_bson_int(char* buf, const char* key, int32_t value);
int write_bson_doclen(char* buf, uint32_t value);
int write_bson_array(char* buf, const char* key);
int write_bson_object(char* buf, const char* key);
int write_bson_string(char* buf, const char* key, const char* value);
int write_bson_boolean(char* buf, const char* key, bool value);
int write_bson_double(char* buf, const char* key, double value);
int write_bson_binary(char* buf, const char* key, const struct void_data* value);

void init_bson_parser();
void shutdown_bson_parser();

#endif /* end of include guard: BSON_PARSER_H */
