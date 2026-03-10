/**
 * @file memory_plugin.c
 * @brief Memory Plugin Implementation
 * @version 1.0.0
 * 
 * Memory read/write/search operations using libmimi-memory.
 * Input is JSON: {"op":"read"|"write"|"search", "key":"...", "value":"..."}
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mimi_tools.h"

/* Forward declarations from libmimi-memory */
typedef void mimi_memory_t;
mimi_memory_t* mimi_memory_create(const char *db_path);
void mimi_memory_destroy(mimi_memory_t *mem);
int mimi_memory_set(mimi_memory_t *mem, const char *key, const char *value);
char* mimi_memory_get(mimi_memory_t *mem, const char *key);
int mimi_memory_delete(mimi_memory_t *mem, const char *key);
int mimi_memory_has(mimi_memory_t *mem, const char *key);

/* ============================================================================
 * Plugin Context
 * ============================================================================ */

typedef struct {
    int initialized;
    mimi_memory_t *memory;
} memory_plugin_ctx_t;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Simple JSON string extraction
 */
static char* json_get_string(const char *json, const char *key)
{
    if (json == NULL || key == NULL) {
        return NULL;
    }
    
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_key);
    if (key_pos == NULL) {
        return NULL;
    }
    
    /* Find the colon */
    const char *colon = strchr(key_pos + strlen(search_key), ':');
    if (colon == NULL) {
        return NULL;
    }
    
    /* Skip whitespace */
    const char *value = colon + 1;
    while (*value == ' ' || *value == '\t' || *value == '\n') {
        value++;
    }
    
    /* Check if it's a string */
    if (*value != '"') {
        return NULL;
    }
    
    value++; /* Skip opening quote */
    
    /* Find closing quote (handle escapes) */
    const char *end = value;
    while (*end != '\0' && *end != '"') {
        if (*end == '\\' && *(end + 1) != '\0') {
            end += 2;
        } else {
            end++;
        }
    }
    
    if (*end != '"') {
        return NULL;
    }
    
    size_t len = end - value;
    char *result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    memcpy(result, value, len);
    result[len] = '\0';
    
    return result;
}

/* ============================================================================
 * Plugin Implementation
 * ============================================================================ */

static int memory_plugin_init(mimi_tool_ctx_t *ctx)
{
    if (ctx == NULL) {
        /* Plugin loaded dynamically - skip context initialization */
        return 0;
    }
    
    memory_plugin_ctx_t *mctx = (memory_plugin_ctx_t*)ctx;
    
    /* Initialize memory store with default path */
    const char *db_path = getenv("MIMI_MEMORY_DB");
    if (db_path == NULL) {
        db_path = "~/.mimiclaw/memory.db";
    }
    
    mctx->memory = mimi_memory_create(db_path);
    if (mctx->memory == NULL) {
        return -1;
    }
    
    mctx->initialized = 1;
    return 0;
}

static int memory_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    memory_plugin_ctx_t *mctx = (memory_plugin_ctx_t*)ctx;
    
    if (mctx == NULL || output == NULL || input == NULL || mctx->memory == NULL) {
        return -1;
    }
    
    /* Parse JSON input */
    char *op = json_get_string(input, "op");
    char *key = json_get_string(input, "key");
    char *value = json_get_string(input, "value");
    
    if (op == NULL || key == NULL) {
        *output = strdup("{\"error\":\"Missing required fields: op, key\"}");
        free(op);
        free(key);
        free(value);
        return *output != NULL ? 0 : -1;
    }
    
    int result = 0;
    
    if (strcmp(op, "read") == 0 || strcmp(op, "get") == 0) {
        char *stored_value = mimi_memory_get(mctx->memory, key);
        if (stored_value == NULL) {
            *output = malloc(256);
            if (*output) {
                sprintf(*output, "{\"error\":\"Key not found\",\"key\":\"%s\",\"success\":false}", key);
            }
        } else {
            /* Escape value for JSON */
            size_t len = strlen(stored_value);
            char *escaped = malloc(len * 2 + 64);
            if (escaped) {
                char *p = escaped;
                p += sprintf(p, "{\"value\":\"");
                for (size_t i = 0; i < len; i++) {
                    char c = stored_value[i];
                    if (c == '"') { p += sprintf(p, "\\\""); }
                    else if (c == '\\') { p += sprintf(p, "\\\\"); }
                    else if (c == '\n') { p += sprintf(p, "\\n"); }
                    else if (c == '\r') { p += sprintf(p, "\\r"); }
                    else if (c == '\t') { p += sprintf(p, "\\t"); }
                    else { *p++ = c; }
                }
                sprintf(p, "\",\"key\":\"%s\",\"success\":true}", key);
                *output = escaped;
            } else {
                *output = strdup("{\"error\":\"Memory allocation failed\"}");
            }
            free(stored_value);
        }
    } else if (strcmp(op, "write") == 0 || strcmp(op, "set") == 0) {
        if (value == NULL) {
            *output = strdup("{\"error\":\"Write operation requires value field\"}");
        } else if (mimi_memory_set(mctx->memory, key, value) == 0) {
            *output = malloc(256);
            if (*output) {
                sprintf(*output, "{\"success\":true,\"key\":\"%s\",\"bytes\":%zu}", 
                        key, strlen(value));
            }
        } else {
            *output = strdup("{\"error\":\"Failed to write memory\",\"success\":false}");
        }
    } else if (strcmp(op, "delete") == 0) {
        if (mimi_memory_delete(mctx->memory, key) == 0) {
            *output = malloc(128);
            if (*output) {
                sprintf(*output, "{\"success\":true,\"key\":\"%s\"}", key);
            }
        } else {
            *output = strdup("{\"error\":\"Failed to delete from memory\",\"success\":false}");
        }
    } else if (strcmp(op, "search") == 0 || strcmp(op, "has") == 0) {
        int exists = mimi_memory_has(mctx->memory, key);
        *output = malloc(128);
        if (*output) {
            sprintf(*output, "{\"exists\":%s,\"key\":\"%s\"}", exists ? "true" : "false", key);
        }
    } else {
        *output = strdup("{\"error\":\"Unknown operation. Use: read, write, delete, search\"}");
    }
    
    free(op);
    free(key);
    free(value);
    
    return *output != NULL ? 0 : -1;
}

static int memory_plugin_destroy(mimi_tool_ctx_t *ctx)
{
    memory_plugin_ctx_t *mctx = (memory_plugin_ctx_t*)ctx;
    
    if (mctx == NULL) {
        return 0;
    }
    
    if (mctx->memory != NULL) {
        mimi_memory_destroy(mctx->memory);
    }
    
    mctx->initialized = 0;
    free(mctx);
    
    return 0;
}

/* ============================================================================
 * Plugin Metadata and Interface
 * ============================================================================ */

static const mimi_plugin_t memory_plugin = {
    .meta = {
        .name = "memory",
        .version = "1.0.0",
        .description = "Memory operations: read, write, delete, search (uses libmimi-memory)",
        .author = "MimiClaw Project"
    },
    .init = memory_plugin_init,
    .exec = memory_plugin_exec,
    .destroy = memory_plugin_destroy
};

/* ============================================================================
 * Exported Symbol
 * ============================================================================ */

const mimi_plugin_t* get_tool_plugin(void)
{
    return &memory_plugin;
}
