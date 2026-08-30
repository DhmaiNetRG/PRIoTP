/**
 * @file logger.h
 * @brief Modular event logging system.
 * 
 * Provides per-module logging with separate channels for normal output,
 * debug trace, and error reporting.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define MODULE_NAME_LEN 30  /**< Maximum module identifier length */

/**
 * @struct logger
 * @brief Logging context for a module.
 */
struct logger
{
  FILE* f_print;                   /**< Standard output stream */
  FILE* f_debug;                   /**< Debug trace stream */
  FILE* f_error;                   /**< Error log stream */
  char module[MODULE_NAME_LEN];    /**< Module identifier */
};

/**
 * @brief Create logger instance.
 * @param print_file Standard output file pointer
 * @param debug_file Debug output file pointer
 * @param error_file Error output file pointer
 * @param module Module identifier string
 * @return Pointer to initialized logger, NULL on error
 */
struct logger* init_logger(FILE *print_file, FILE *debug_file, FILE *error_file,
                           const char* module);

/**
 * @brief Write message to standard output.
 * @param l Logger instance
 * @param format Printf-style format string
 */
void log_print(const struct logger* l, const char* format, ...);

/**
 * @brief Write message to debug trace.
 * @param l Logger instance
 * @param format Printf-style format string
 */
void log_debug(const struct logger* l, const char* format, ...);

/**
 * @brief Write message to error log.
 * @param l Logger instance
 * @param format Printf-style format string
 */
void log_error(const struct logger* l, const char* format, ...);

/**
 * @brief Shutdown and cleanup logger.
 * @param l Logger instance
 */
void shutdown_logger(struct logger* l);

#endif

