/**
 * @file sensor_logger.h
 * @brief Sensor measurement logging and archival.
 * 
 * Provides per-sensor logging and binary dump facilities for
 * measurement analysis, validation, and offline processing.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 */

#ifndef SENSOR_LOGGER_H
#define SENSOR_LOGGER_H

#include "logger.h"
#include "sensor_types.h"
#include <stdbool.h>

/**
 * @struct sensor_logger
 * @brief Logger instance for a sensor.
 */
struct sensor_logger
{
  char* id;                        /**< Sensor identifier */
  struct logger* logger;           /**< Logger instance */
  struct sensor_logger* next;      /**< Linked list pointer */
};

/**
 * @struct sensor_dump
 * @brief Binary data archival for sensor.
 */
struct sensor_dump
{
  char* id;                        /**< Sensor identifier */
  FILE* dump;                      /**< Binary dump file stream */
  struct sensor_dump* next;        /**< Linked list pointer */
};

/**
 * @brief Create binary dump file for sensor.
 * @param id Sensor identifier
 * @return Pointer to dump entry, NULL on error
 */
struct sensor_dump* add_sensor_dump(const char* id);

/**
 * @brief Create logger for sensor.
 * @param id Sensor identifier
 * @return Pointer to logger entry, NULL on error
 */
struct sensor_logger* add_sensor_logger(const char* id);

/**
 * @brief Log sensor measurement to text file.
 * @param id Sensor identifier
 * @param seq_no Sequence number
 * @param type Sensor type
 * @param data Measurement payload
 */
void sensor_log(const char* id, int seq_no, enum SENSOR_TYPE type, const struct void_data* data);

/**
 * @brief Archive sensor data to binary dump file.
 * @param id Sensor identifier
 * @param data Payload bytes to archive
 */
void sensor_dump(const char* id, const struct void_data* data);

/**
 * @brief Initialize sensor logger module.
 */
void init_sensor_logger();

/**
 * @brief Shutdown sensor logger module.
 */
void shutdown_sensor_logger();

#endif

