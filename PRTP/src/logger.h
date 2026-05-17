#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define MODULE_NAME_LEN 30

struct logger
{
  FILE* f_print;
  FILE* f_debug;
  FILE* f_error;
  char module[MODULE_NAME_LEN];
};

struct logger* init_logger(FILE *print_file, FILE *debug_file, FILE *error_file,
                           const char* module);
void log_print(const struct logger* l, const char* format, ...);
void log_debug(const struct logger* l, const char* format, ...);
void log_error(const struct logger* l, const char* format, ...);
void shutdown_logger(struct logger* l);

#endif

