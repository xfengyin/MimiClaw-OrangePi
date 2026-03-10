/**
 * @file file_ops_plugin.c
 * @brief File Operations Plugin Implementation
 * @version 1.0.0
 * 
 * Provides file read, write, and delete operations.
 * Input is JSON: {"op":"read"|"write"|"delete", "path":"...", "content":"..."}
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "mimi_tools.h"

/* ============================================================================
 * Plugin Context
 * ============================================================================ */

typedef struct {
    int initialized;
    char *base_dir;
} file_ops_plugin_ctx_t;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Read file contents into a string
 */
static char* read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return NULL;
    }
    
    /* Get file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    
    /* Allocate buffer */
    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fclose(f);
        return NULL;
    }
    
    /* Read file */
    size_t read_size = fread(buffer, 1, size, f);
    buffer[read_size] = '\0';
    
    fclose(f);
    return buffer;
}

/**
 * @brief Write string to file
 */
static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    
    if (fprintf(f, "%s", content) < 0) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return 0;
}

/**
 * @brief Delete a file
 */
static int delete_file(const char *path)
{
    return unlink(path);
}

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

static int file_ops_plugin_init(mimi_tool_ctx_t *ctx)
{
    if (ctx == NULL) {
        /* Plugin loaded dynamically - skip context initialization */
        return 0;
    }
    
    file_ops_plugin_ctx_t *fctx = (file_ops_plugin_ctx_t*)ctx;
    
    /* Default to current directory */
    fctx->base_dir = getcwd(NULL, 0);
    if (fctx->base_dir == NULL) {
        fctx->base_dir = strdup(".");
    }
    
    fctx->initialized = 1;
    return 0;
}

static int file_ops_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    file_ops_plugin_ctx_t *fctx = (file_ops_plugin_ctx_t*)ctx;
    
    if (fctx == NULL || output == NULL || input == NULL) {
        return -1;
    }
    
    /* Parse JSON input */
    char *op = json_get_string(input, "op");
    char *path = json_get_string(input, "path");
    char *content = json_get_string(input, "content");
    
    if (op == NULL || path == NULL) {
        *output = strdup("{\"error\":\"Missing required fields: op, path\"}");
        free(op);
        free(path);
        free(content);
        return *output != NULL ? 0 : -1;
    }
    
    /* Resolve path (simple - just use as-is for now) */
    char full_path[1024];
    if (path[0] == '/') {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", fctx->base_dir, path);
    }
    
    int result = 0;
    
    if (strcmp(op, "read") == 0) {
        char *file_content = read_file(full_path);
        if (file_content == NULL) {
            *output = strdup("{\"error\":\"Failed to read file\",\"success\":false}");
        } else {
            /* Escape content for JSON */
            size_t len = strlen(file_content);
            char *escaped = malloc(len * 2 + 64);
            if (escaped) {
                char *p = escaped;
                p += sprintf(p, "{\"content\":\"");
                for (size_t i = 0; i < len; i++) {
                    char c = file_content[i];
                    if (c == '"') { p += sprintf(p, "\\\""); }
                    else if (c == '\\') { p += sprintf(p, "\\\\"); }
                    else if (c == '\n') { p += sprintf(p, "\\n"); }
                    else if (c == '\r') { p += sprintf(p, "\\r"); }
                    else if (c == '\t') { p += sprintf(p, "\\t"); }
                    else { *p++ = c; }
                }
                sprintf(p, "\",\"success\":true,\"path\":\"%s\"}", full_path);
                *output = escaped;
            } else {
                *output = strdup("{\"error\":\"Memory allocation failed\"}");
            }
            free(file_content);
        }
    } else if (strcmp(op, "write") == 0) {
        if (content == NULL) {
            *output = strdup("{\"error\":\"Write operation requires content field\"}");
        } else if (write_file(full_path, content) == 0) {
            *output = malloc(256);
            if (*output) {
                sprintf(*output, "{\"success\":true,\"path\":\"%s\",\"bytes\":%zu}", 
                        full_path, strlen(content));
            }
        } else {
            *output = strdup("{\"error\":\"Failed to write file\",\"success\":false}");
        }
    } else if (strcmp(op, "delete") == 0) {
        if (delete_file(full_path) == 0) {
            *output = malloc(128);
            if (*output) {
                sprintf(*output, "{\"success\":true,\"path\":\"%s\"}", full_path);
            }
        } else {
            *output = strdup("{\"error\":\"Failed to delete file\",\"success\":false}");
        }
    } else {
        *output = strdup("{\"error\":\"Unknown operation. Use: read, write, delete\"}");
    }
    
    free(op);
    free(path);
    free(content);
    
    return *output != NULL ? 0 : -1;
}

static int file_ops_plugin_destroy(mimi_tool_ctx_t *ctx)
{
    file_ops_plugin_ctx_t *fctx = (file_ops_plugin_ctx_t*)ctx;
    
    if (fctx == NULL) {
        return 0;
    }
    
    free(fctx->base_dir);
    fctx->initialized = 0;
    free(fctx);
    
    return 0;
}

/* ============================================================================
 * Plugin Metadata and Interface
 * ============================================================================ */

static const mimi_plugin_t file_ops_plugin = {
    .meta = {
        .name = "file-ops",
        .version = "1.0.0",
        .description = "File operations: read, write, delete (JSON input)",
        .author = "MimiClaw Project"
    },
    .init = file_ops_plugin_init,
    .exec = file_ops_plugin_exec,
    .destroy = file_ops_plugin_destroy
};

/* ============================================================================
 * Exported Symbol
 * ============================================================================ */

const mimi_plugin_t* get_tool_plugin(void)
{
    return &file_ops_plugin;
}
