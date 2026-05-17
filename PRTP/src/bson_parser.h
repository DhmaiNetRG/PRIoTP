/**
 * @file bson_parser.h
 * @brief BSON binary format parsing and serialization.
 * 
 * Low-level BSON codec providing document parsing and element writing
 * for flexible, schema-free message encoding in PRIoTP.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef BSON_PARSER_H

#define BSON_PARSER_H

#include <stdbool.h>
#include "messages.h"

/**
 * @enum BSON_TYPE
 * @brief BSON element type tags.
 */
enum BSON_TYPE {
    BSON_UNKNOWN__,     /**< Unrecognized type */
    BSON_DOUBLE,        /**< 64-bit IEEE floating point */
    BSON_STRING,        /**< UTF-8 string */
    BSON_DOCUMENT,      /**< Nested document */
    BSON_ARRAY,         /**< Array of elements */
    BSON_BINARY,        /**< Binary data */
    BSON_UNKNOWN_6,     /**< Reserved */
    BSON_OBJECTID,      /**< Object identifier */
    BSON_BOOLEAN,       /**< Boolean value */
    BSON_UTCDATETIME,   /**< UTC timestamp */
    BSON_NULL,          /**< Null value */
    BSON_REGEXP,        /**< Regular expression */
    BSON_UNKNOWN_12,    /**< Reserved */
    BSON_JSCODE,        /**< JavaScript code */
    BSON_UNKNOWN_14,    /**< Reserved */
    BSON_JSCODE_W_S,    /**< JavaScript with scope */
    BSON_INT32,         /**< 32-bit signed integer */
    BSON_TIMESTAMP,     /**< Timestamp (internal use) */
    BSON_INT64          /**< 64-bit signed integer */
};

/**
 * @brief Parse BSON document to PRIoTP packet structure.
 * @param buf BSON binary buffer
 * @param len Buffer length
 * @param result Pointer to receive parsed packet
 * @return Bytes consumed, negative on error
 */
int parse_bson_document(char* buf, size_t len, struct PRTP_packet** result);

/**
 * @brief Write 32-bit integer to BSON buffer.
 * @param buf Output buffer
 * @param key Element key name
 * @param value Integer value to write
 * @return Bytes written, negative on error
 */
int write_bson_int(char* buf, const char* key, int32_t value);

/**
 * @brief Write BSON document length header.
 * @param buf Output buffer
 * @param value Document length in bytes
 * @return Bytes written
 */
int write_bson_doclen(char* buf, uint32_t value);

/**
 * @brief Begin BSON array element.
 * @param buf Output buffer
 * @param key Array key name
 * @return Bytes written
 */
int write_bson_array(char* buf, const char* key);

/**
 * @brief Begin BSON object element.
 * @param buf Output buffer
 * @param key Object key name
 * @return Bytes written
 */
int write_bson_object(char* buf, const char* key);

/**
 * @brief Write UTF-8 string element to BSON.
 * @param buf Output buffer
 * @param key Element key name
 * @param value String value to write
 * @return Bytes written, negative on error
 */
int write_bson_string(char* buf, const char* key, const char* value);

/**
 * @brief Write boolean element to BSON.
 * @param buf Output buffer
 * @param key Element key name
 * @param value Boolean value to write
 * @return Bytes written
 */
int write_bson_boolean(char* buf, const char* key, bool value);

/**
 * @brief Write double-precision element to BSON.
 * @param buf Output buffer
 * @param key Element key name
 * @param value Double value to write
 * @return Bytes written
 */
int write_bson_double(char* buf, const char* key, double value);

/**
 * @brief Write binary data element to BSON.
 * @param buf Output buffer
 * @param key Element key name
 * @param value Binary payload to write
 * @return Bytes written, negative on error
 */
int write_bson_binary(char* buf, const char* key, const struct void_data* value);

/**
 * @brief Initialize BSON parser module.
 */
void init_bson_parser();

/**
 * @brief Shutdown BSON parser module.
 */
void shutdown_bson_parser();

#endif /* end of include guard: BSON_PARSER_H */
