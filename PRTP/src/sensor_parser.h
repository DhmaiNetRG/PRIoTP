/**
 * @file sensor_parser.h
 * @brief Sensor data acquisition and parsing.
 * 
 * Parses heterogeneous sensor updates from JSON-encoded packets,
 * extracting metadata (ID, sequence, timestamp) and sensor-specific payloads.
 * Supports cameras, thermometers, accelerometers, and extensible sensor types.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 */

#ifndef SENSOR_PARSER_H
#define SENSOR_PARSER_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include "logger.h"
#include "void_data.h"

#include "sensor_types.h"

#define BUF_SIZE 5500  /**< Maximum sensor packet buffer (supports video streams) */

/**
 * @struct sensor
 * @brief Parsed sensor measurement data.
 * 
 * Encapsulates sensor metadata and payload with type-specific
 * parsing and formatting functions.
 */
struct sensor
{
  enum SENSOR_TYPE type;                       /**< Sensor modality classification */
  char id[ID_SIZE + 1];                        /**< Unique sensor identifier */
  uint32_t seq_no;                             /**< Update sequence number */
  struct timeval ts;                           /**< Measurement timestamp */
  struct void_data data;                       /**< Sensor payload (type-specific) */
  int (*parse_data)(struct sensor*, const char*);  /**< Type-specific parser function */
  void (*print_data)(struct logger* log, const struct void_data* data); /**< Type-specific formatter */
};

/**
 * @brief Parse raw sensor update packet.
 * 
 * Extracts metadata and delegates payload parsing to type-specific handler.
 * 
 * @param buf Null-terminated JSON string from sensor
 * @param len Buffer length (max BUF_SIZE)
 * @param res Parsed sensor output structure
 * @return 0 on success, negative on parse error
 */
int parse_sensor_update(const char* buf, ssize_t len, struct sensor* res);

/**
 * @brief Display parsed sensor measurement.
 * @param sensor Sensor structure to display
 */
void print_sensor(const struct sensor* sensor);

/**
 * @brief Initialize sensor parser module.
 */
void init_sensor_parser();

/**
 * @brief Shutdown sensor parser module.
 */
void shutdown_sensor_parser();

#endif
