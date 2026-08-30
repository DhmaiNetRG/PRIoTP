#ifndef VOID_DATA_H
#define VOID_DATA_H

struct void_data {
  uint32_t sensor_type;
  char* sid;
  int len;
  void* blob;
};

#endif
