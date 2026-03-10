/**
 * @file echo_plugin.c
 * @brief Echo Plugin Implementation
 * @version 1.0.0
 * 
 * Simple test plugin that echoes input back.
 * Useful for testing the plugin system.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mimi_tools.h"

/* ============================================================================
 * Plugin Context
 * ============================================================================ */

typedef struct {
    int initialized;
} echo_plugin_ctx_t;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Simple JSON string escape
 */
static char* json_escape(const char *input)
{
    if (input == NULL) {
        return strdup("null");
    }
    
    size_t len = strlen(input);
    char *escaped = (char*)malloc(len * 2 + 4);
    if (escaped == NULL) {
        return NULL;
    }
    
    char *p = escaped;
    *p++ = '"';
    
    for (size_t i = 0; i < len; i++) {
        char c = input[i];
        switch (c) {
            case '"':
                *p++ = '\\';
                *p++ = '"';
                break;
            case '\\':
                *p++ = '\\';
                *p++ = '\\';
                break;
            case '\n':
                *p++ = '\\';
                *p++ = 'n';
                break;
            case '\r':
                *p++ = '\\';
                *p++ = 'r';
                break;
            case '\t':
                *p++ = '\\';
                *p++ = 't';
                break;
            case '\b':
                *p++ = '\\';
                *p++ = 'b';
                break;
            case '\f':
                *p++ = '\\';
                *p++ = 'f';
                break;
            default:
                if ((unsigned char)c < 0x20) {
                    /* Control character - encode as \u00XX */
                    p += sprintf(p, "\\u%04x", (unsigned char)c);
                } else {
                    *p++ = c;
                }
                break;
        }
    }
    
    *p++ = '"';
    *p = '\0';
    
    return escaped;
}

/* ============================================================================
 * Plugin Implementation
 * ============================================================================ */

static int echo_plugin_init(mimi_tool_ctx_t *ctx)
{
    if (ctx != NULL) {
        echo_plugin_ctx_t *ectx = (echo_plugin_ctx_t*)ctx;
        ectx->initialized = 1;
    }
    return 0;
}

static int echo_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    (void)ctx;
    
    if (output == NULL) {
        return -1;
    }
    
    char *escaped = json_escape(input);
    if (escaped == NULL) {
        return -1;
    }
    
    /* Build JSON response */
    size_t len = strlen(escaped);
    *output = (char*)malloc(len + 16);
    if (*output == NULL) {
        free(escaped);
        return -1;
    }
    
    sprintf(*output, "{\"echo\":%s}", escaped);
    free(escaped);
    
    return 0;
}

static int echo_plugin_destroy(mimi_tool_ctx_t *ctx)
{
    echo_plugin_ctx_t *ectx = (echo_plugin_ctx_t*)ctx;
    ectx->initialized = 0;
    free(ectx);
    return 0;
}

/* ============================================================================
 * Plugin Metadata and Interface
 * ============================================================================ */

static const mimi_plugin_t echo_plugin = {
    .meta = {
        .name = "echo",
        .version = "1.0.0",
        .description = "Echo input back (useful for testing the plugin system)",
        .author = "MimiClaw Project"
    },
    .init = echo_plugin_init,
    .exec = echo_plugin_exec,
    .destroy = echo_plugin_destroy
};

/* ============================================================================
 * Exported Symbol
 * ============================================================================ */

const mimi_plugin_t* get_tool_plugin(void)
{
    return &echo_plugin;
}
