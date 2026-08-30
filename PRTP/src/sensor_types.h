#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H 
#include <stdint.h>
#include "logger.h"

#include "void_data.h"

#define ID_SIZE 20

/* XXX Not following specification here, 0 is for unknown */
enum SENSOR_TYPE {
  UNKNOWN,
  TEMP,
  DEVICE,
  GPS,
  CAMERA
};

void sensor_type_to_string(enum SENSOR_TYPE type, char* res);
enum SENSOR_TYPE parse_sensor_type(const char* buf);

/* Printers of sensor data */
void print_sensor_data(struct logger* log, enum SENSOR_TYPE type, const struct void_data* data);
void print_device_data(struct logger* log, const struct void_data* data);
void print_temperature_data(struct logger* log, const struct void_data* data);
void print_camera_data(struct logger* log, const struct void_data* data);
void print_gps_data(struct logger* log, const struct void_data* data);
void print_unknown_data(struct logger* log, const struct void_data* data);

/* Allocate buffer for sensor data of the specified type */
int sensor_alloc_data(enum SENSOR_TYPE type, struct void_data* data);

/* Copy sensor data according to the type */
int sensor_copy_data(enum SENSOR_TYPE type, struct void_data* to, const struct void_data* from);

/* Copy sensor data according to the type with the offset given*/
int sensor_copy_offset_data(enum SENSOR_TYPE type, int offset, struct void_data* to,
                            const struct void_data* from);

/* Allocate + copy = set, instead of data range use copy offset */
int sensor_set_data(enum SENSOR_TYPE type, struct void_data* to, const struct void_data* value);
/*int sensor_set_data_range(enum SENSOR_TYPE type, void** to, const void* value, uint32_t offset, uint32_t length);
*/
void init_sensor_types();
void shutdown_sensor_types();

#endif
