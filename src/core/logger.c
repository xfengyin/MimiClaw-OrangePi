/*
 * Logging System Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "logger.h"

static struct {
    FILE *fp;
    log_level_t level;
    pthread_mutex_t mutex;
    bool initialized;
} g_logger = {NULL, LOG_LEVEL_INFO, PTHREAD_MUTEX_INITIALIZER, false};

static const char *level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    "\033[36m", "\033[32m", "\033[33m", "\033[31m", "\033[35m"
};

int logger_init(const char *log_file, log_level_t level) {
    pthread_mutex_lock(&g_logger.mutex);
    
    if (g_logger.initialized) {
        pthread_mutex_unlock(&g_logger.mutex);
        return 0;
    }
    
    g_logger.level = level;
    
    if (log_file) {
        g_logger.fp = fopen(log_file, "a");
        if (!g_logger.fp) {
            pthread_mutex_unlock(&g_logger.mutex);
            return -1;
        }
    } else {
        g_logger.fp = stdout;
    }
    
    g_logger.initialized = true;
    pthread_mutex_unlock(&g_logger.mutex);
    
    LOG_INFO("Logger initialized, level: %s", level_strings[level]);
    return 0;
}

void logger_close(void) {
    pthread_mutex_lock(&g_logger.mutex);
    
    if (!g_logger.initialized) {
        pthread_mutex_unlock(&g_logger.mutex);
        return;
    }
    
    if (g_logger.fp && g_logger.fp != stdout) {
        fclose(g_logger.fp);
    }
    
    g_logger.fp = NULL;
    g_logger.initialized = false;
    
    pthread_mutex_unlock(&g_logger.mutex);
}

void logger_set_level(log_level_t level) {
    pthread_mutex_lock(&g_logger.mutex);
    g_logger.level = level;
    pthread_mutex_unlock(&g_logger.mutex);
}

log_level_t logger_get_level_from_string(const char *level_str) {
    if (!level_str) return LOG_LEVEL_INFO;
    
    if (strcasecmp(level_str, "debug") == 0) return LOG_LEVEL_DEBUG;
    if (strcasecmp(level_str, "info") == 0) return LOG_LEVEL_INFO;
    if (strcasecmp(level_str, "warn") == 0) return LOG_LEVEL_WARN;
    if (strcasecmp(level_str, "error") == 0) return LOG_LEVEL_ERROR;
    if (strcasecmp(level_str, "fatal") == 0) return LOG_LEVEL_FATAL;
    
    return LOG_LEVEL_INFO;
}

void logger_log(log_level_t level, const char *file, int line, const char *fmt, va_list args) {
    if (!g_logger.initialized || level < g_logger.level) {
        return;
    }
    
    pthread_mutex_lock(&g_logger.mutex);
    
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Extract filename only
    const char *filename = strrchr(file, '/');
    if (filename) filename++;
    else filename = file;
    
    // Print to console with colors
    if (g_logger.fp == stdout) {
        fprintf(stdout, "%s[%s] %s%-5s\033[0m \033[90m%s:%d\033[0m ", 
            time_buf, 
            level_colors[level],
            level_strings[level],
            filename, 
            line);
        vfprintf(stdout, fmt, args);
        fprintf(stdout, "\n");
    } else {
        // Print to file without colors
        fprintf(g_logger.fp, "[%s] %-5s %s:%d ", time_buf, level_strings[level], filename, line);
        vfprintf(g_logger.fp, fmt, args);
        fprintf(g_logger.fp, "\n");
        fflush(g_logger.fp);
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
}

void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, args);
    va_end(args);
}

void log_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, args);
    va_end(args);
}