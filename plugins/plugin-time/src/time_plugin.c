/**
 * @file time_plugin.c
 * @brief Time Plugin Implementation
 * @version 1.0.0
 * 
 * Provides current date/time in various formats:
 * - "now" or empty: Full ISO timestamp
 * - "date": Date only (YYYY-MM-DD)
 * - "time": Time only (HH:MM:SS)
 * - "unix": Unix timestamp
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mimi_tools.h"

/* ============================================================================
 * Plugin Context
 * ============================================================================ */

typedef struct {
    int initialized;
} time_plugin_ctx_t;

/* ============================================================================
 * Plugin Implementation
 * ============================================================================ */

static int time_plugin_init(mimi_tool_ctx_t *ctx)
{
    if (ctx != NULL) {
        time_plugin_ctx_t *tctx = (time_plugin_ctx_t*)ctx;
        tctx->initialized = 1;
    }
    return 0;
}

static int time_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    (void)ctx;
    
    if (output == NULL) {
        return -1;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char buffer[256];
    
    /* Parse input for format selection */
    const char *fmt = "iso";
    if (input != NULL && strlen(input) > 0) {
        if (strstr(input, "date") != NULL) fmt = "date";
        else if (strstr(input, "time") != NULL) fmt = "time";
        else if (strstr(input, "unix") != NULL) fmt = "unix";
    }
    
    if (strcmp(fmt, "unix") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"timestamp\":%ld}", (long)now);
    } else if (strcmp(fmt, "date") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"date\":\"%04d-%02d-%02d\"}",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    } else if (strcmp(fmt, "time") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"time\":\"%02d:%02d:%02d\"}",
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        /* Default: full ISO format */
        snprintf(buffer, sizeof(buffer),
            "{\"timestamp\":%ld,\"iso\":\"%04d-%02d-%02dT%02d:%02d:%02d+08:00\",\"human\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
            (long)now,
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec
        );
    }
    
    *output = strdup(buffer);
    return (*output != NULL) ? 0 : -1;
}

static int time_plugin_destroy(mimi_tool_ctx_t *ctx)
{
    time_plugin_ctx_t *tctx = (time_plugin_ctx_t*)ctx;
    tctx->initialized = 0;
    free(tctx);
    return 0;
}

/* ============================================================================
 * Plugin Metadata and Interface
 * ============================================================================ */

static const mimi_plugin_t time_plugin = {
    .meta = {
        .name = "time",
        .version = "1.0.0",
        .description = "Get current date and time in various formats (now, date, time, unix)",
        .author = "MimiClaw Project"
    },
    .init = time_plugin_init,
    .exec = time_plugin_exec,
    .destroy = time_plugin_destroy
};

/* ============================================================================
 * Exported Symbol
 * ============================================================================ */

const mimi_plugin_t* get_tool_plugin(void)
{
    return &time_plugin;
}
