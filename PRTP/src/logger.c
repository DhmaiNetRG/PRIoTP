#include "logger.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct logger* init_logger(FILE* print_file, FILE* debug_file, FILE* error_file,
                           const char* module)
{
  struct logger* l = malloc(sizeof(struct logger));
  l->f_print = print_file;
  l->f_debug = debug_file;
  l->f_error = error_file;
  strcpy(l->module, module);
  return l;
}

void log_print(const struct logger* l, const char* format, ...)
{
  va_list arg;
  
  if (l == NULL) return;
  if (l->f_print == NULL) return;
  
  va_start(arg, format);
  vfprintf(l->f_print, format, arg);
  va_end(arg);
}

/* ENABLE DEBUG LOGGING - Uncomment this line to see Q-agent decisions */
#define DEBUG_LOG

void log_debug(const struct logger* l, const char* format, ...)
{
#ifdef DEBUG_LOG
  va_list arg;
  struct timeval now;
  
  if (l == NULL) return;
  if (l->f_debug == NULL) return;
  
  gettimeofday(&now, 0);
  fprintf(l->f_debug, "\033[95m [%u.%02u] %s: \033[0m",
          (unsigned int)now.tv_sec, (unsigned int)now.tv_usec, l->module);

  va_start(arg, format);
  vfprintf(l->f_debug, format, arg);
  va_end(arg);
  fprintf(l->f_debug, "\033[0m");
#endif
}

void log_error(const struct logger* l, const char* format, ...)
{
  va_list arg;
  struct timeval now;
  
  if (l == NULL) return;
  if (l->f_error == NULL) return;
  
  gettimeofday(&now, 0);
  fprintf(l->f_error, "\033[91m [%u.%02u] %s: \033[0m",
          (unsigned int)now.tv_sec, (unsigned int)now.tv_usec, l->module);
  va_start(arg, format);
  vfprintf(l->f_error, format, arg);
  va_end(arg);
}

void shutdown_logger(struct logger* l)
{
  free(l);
}