/*
 * Logging System Header
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

// Initialize logger
int logger_init(const char *log_file, log_level_t level);
void logger_close(void);

// Set log level
void logger_set_level(log_level_t level);
log_level_t logger_get_level_from_string(const char *level_str);

// Logging functions
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_fatal(const char *fmt, ...);

// Internal function (use macros above)
void logger_log(log_level_t level, const char *file, int line, const char *fmt, va_list args);

// Macros for convenient logging
#define LOG_DEBUG(fmt, ...) log_debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) log_info(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) log_warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_error(fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_fatal(fmt, ##__VA_ARGS__)

#endif // LOGGER_H