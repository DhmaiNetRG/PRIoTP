/**
 * @file bson_msg.h
 * @brief BSON serialization and deserialization for PRIoTP messages.
 * 
 * Provides Binary JSON (BSON) codec for efficient message encoding and
 * structured data extraction from PRIoTP packets.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef BSON_MSG_H

#define BSON_MSG_H

#include <stdbool.h>
#include "messages.h"

/**
 * @brief Serialize PRIoTP packet to BSON binary format.
 * @param msg PRIoTP packet to serialize
 * @param buf Output buffer for BSON data
 * @param maxlen Maximum buffer capacity
 * @return Number of bytes written, negative on error
 */
int serialize_iotmsg(const struct PRTP_packet* msg, char* buf, uint32_t maxlen);

/**
 * @brief Extract string field from BSON message.
 * @param msg Pointer to PRIoTP packet structure
 * @param parent Parent field name
 * @param key Field key
 * @param value String value to extract
 * @return Status code
 */
int bson_iotmsg_string(struct PRTP_packet** msg, char* parent, char* key, char* value);

/**
 * @brief Extract boolean field from BSON message.
 * @param msg Pointer to PRIoTP packet structure
 * @param parent Parent field name
 * @param key Field key
 * @param value Boolean value to extract
 * @return Status code
 */
int bson_iotmsg_boolean(struct PRTP_packet** msg, char* parent, char* key, bool value);

/**
 * @brief Extract 32-bit integer field from BSON message.
 * @param msg Pointer to PRIoTP packet structure
 * @param parent Parent field name
 * @param key Field key
 * @param value Integer value to extract
 * @return Status code
 */
int bson_iotmsg_int32(struct PRTP_packet** msg, char* parent, char* key, int32_t value);

/**
 * @brief Extract double-precision field from BSON message.
 * @param msg Pointer to PRIoTP packet structure
 * @param parent Parent field name
 * @param key Field key
 * @param value Double value to extract
 * @return Status code
 */
int bson_iotmsg_double(struct PRTP_packet** msg, char* parent, char* key, double value);

/**
 * @brief Extract binary data field from BSON message.
 * @param msg Pointer to PRIoTP packet structure
 * @param parent Parent field name
 * @param key Field key
 * @param data Binary data to extract
 * @return Status code
 */
int bson_iotmsg_binary(struct PRTP_packet** msg, char* parent, char* key, void* data);

/**
 * @brief Initialize BSON message codec.
 */
void init_bson_msg();

/**
 * @brief Shutdown BSON message codec.
 */
void shutdown_bson_msg();

#endif /* end of include guard: BSON_MSG_H */
