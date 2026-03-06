/*
 * File Tools Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <json-c/json.h>
#include "tool_registry.h"
#include "../core/logger.h"

#define MAX_FILE_SIZE (1024 * 1024) // 1MB

int tool_read_file(const char *params, char *result, size_t result_size) {
    if (!params || !result || result_size == 0) return -1;
    
    // Parse params
    json_object *params_obj = json_tokener_parse(params);
    if (!params_obj) {
        strncpy(result, "Error: Invalid parameters", result_size - 1);
        return -1;
    }
    
    json_object *path_obj;
    if (!json_object_object_get_ex(params_obj, "path", &path_obj)) {
        strncpy(result, "Error: Missing 'path' parameter", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    const char *path = json_object_get_string(path_obj);
    LOG_INFO("Reading file: %s", path);
    
    // Security check - only allow reading from safe directories
    if (strstr(path, "..") || strstr(path, "~")) {
        strncpy(result, "Error: Path contains invalid characters", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        snprintf(result, result_size, "Error: File not found: %s", path);
        json_object_put(params_obj);
        return -1;
    }
    
    if (st.st_size > MAX_FILE_SIZE) {
        strncpy(result, "Error: File too large (max 1MB)", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        snprintf(result, result_size, "Error: Cannot open file: %s", path);
        json_object_put(params_obj);
        return -1;
    }
    
    // Read file content
    char *content = malloc(st.st_size + 1);
    if (!content) {
        strncpy(result, "Error: Memory allocation failed", result_size - 1);
        fclose(fp);
        json_object_put(params_obj);
        return -1;
    }
    
    size_t read_size = fread(content, 1, st.st_size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    // Build result
    snprintf(result, result_size, "Content of %s:\n\n%s", path, content);
    free(content);
    
    json_object_put(params_obj);
    return 0;
}

int tool_write_file(const char *params, char *result, size_t result_size) {
    if (!params || !result || result_size == 0) return -1;
    
    // Parse params
    json_object *params_obj = json_tokener_parse(params);
    if (!params_obj) {
        strncpy(result, "Error: Invalid parameters", result_size - 1);
        return -1;
    }
    
    json_object *path_obj;
    json_object *content_obj;
    
    if (!json_object_object_get_ex(params_obj, "path", &path_obj) ||
        !json_object_object_get_ex(params_obj, "content", &content_obj)) {
        strncpy(result, "Error: Missing 'path' or 'content' parameter", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    const char *path = json_object_get_string(path_obj);
    const char *content = json_object_get_string(content_obj);
    
    LOG_INFO("Writing file: %s", path);
    
    // Security check
    if (strstr(path, "..") || strstr(path, "~")) {
        strncpy(result, "Error: Path contains invalid characters", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    // Only allow writing to /tmp and /home/orangepi
    if (strncmp(path, "/tmp/", 5) != 0 && strncmp(path, "/home/orangepi/", 15) != 0) {
        strncpy(result, "Error: Can only write to /tmp or /home/orangepi/", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        snprintf(result, result_size, "Error: Cannot create file: %s", path);
        json_object_put(params_obj);
        return -1;
    }
    
    size_t written = fwrite(content, 1, strlen(content), fp);
    fclose(fp);
    
    if (written != strlen(content)) {
        snprintf(result, result_size, "Error: Failed to write complete content to %s", path);
        json_object_put(params_obj);
        return -1;
    }
    
    snprintf(result, result_size, "Successfully wrote %zu bytes to %s", written, path);
    
    json_object_put(params_obj);
    return 0;
}