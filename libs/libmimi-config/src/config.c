/**
 * @file config.c
 * @brief MimiClaw Config Library Implementation
 * @version 2.0.0
 */

#define _GNU_SOURCE

#include "mimi_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#ifdef __linux__
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* ============================================================================
 * Simple JSON Parser (Minimal Implementation)
 * ============================================================================ */

typedef struct json_value json_value_t;

struct json_value {
    mimi_config_type_t type;
    union {
        char *string_val;
        int int_val;
        double float_val;
        int bool_val;
        struct {
            json_value_t **items;
            int count;
        } array;
        struct {
            char **keys;
            json_value_t **values;
            int count;
        } object;
    } data;
};

struct mimi_config {
    char *path;
    json_value_t *root;
    int64_t mtime;
    int watch_fd;
    mimi_config_options_t watch_options;
    int watching;
};

/* Forward declarations */
static json_value_t* parse_value(const char **json);
static void free_value(json_value_t *val);

static void skip_whitespace(const char **json)
{
    while (**json == ' ' || **json == '\t' || **json == '\n' || **json == '\r') {
        (*json)++;
    }
}

static char* parse_string_raw(const char **json)
{
    if (**json != '"') return NULL;
    (*json)++;
    
    const char *start = *json;
    while (**json != '"' && **json != '\0') {
        if (**json == '\\') (*json)++;
        if (**json != '\0') (*json)++;
    }
    
    size_t len = *json - start;
    char *result = (char*)malloc(len + 1);
    if (result == NULL) return NULL;
    
    /* Simple copy (doesn't handle all escape sequences) */
    memcpy(result, start, len);
    result[len] = '\0';
    
    if (**json == '"') (*json)++;
    return result;
}

static json_value_t* parse_string(const char **json)
{
    char *str = parse_string_raw(json);
    if (str == NULL) return NULL;
    
    json_value_t *val = (json_value_t*)calloc(1, sizeof(json_value_t));
    if (val == NULL) {
        free(str);
        return NULL;
    }
    
    val->type = MIMI_CONFIG_TYPE_STRING;
    val->data.string_val = str;
    return val;
}

static json_value_t* parse_number(const char **json)
{
    const char *start = *json;
    int is_float = 0;
    
    if (**json == '-') (*json)++;
    
    while (**json >= '0' && **json <= '9') (*json)++;
    
    if (**json == '.') {
        is_float = 1;
        (*json)++;
        while (**json >= '0' && **json <= '9') (*json)++;
    }
    
    if (**json == 'e' || **json == 'E') {
        is_float = 1;
        (*json)++;
        if (**json == '+' || **json == '-') (*json)++;
        while (**json >= '0' && **json <= '9') (*json)++;
    }
    
    size_t len = *json - start;
    char *num_str = (char*)malloc(len + 1);
    if (num_str == NULL) return NULL;
    
    memcpy(num_str, start, len);
    num_str[len] = '\0';
    
    json_value_t *val = (json_value_t*)calloc(1, sizeof(json_value_t));
    if (val == NULL) {
        free(num_str);
        return NULL;
    }
    
    if (is_float) {
        val->type = MIMI_CONFIG_TYPE_FLOAT;
        val->data.float_val = atof(num_str);
    } else {
        val->type = MIMI_CONFIG_TYPE_INTEGER;
        val->data.int_val = atoi(num_str);
    }
    
    free(num_str);
    return val;
}

static json_value_t* parse_object(const char **json)
{
    if (**json != '{') return NULL;
    (*json)++;
    
    json_value_t *obj = (json_value_t*)calloc(1, sizeof(json_value_t));
    if (obj == NULL) return NULL;
    
    obj->type = MIMI_CONFIG_TYPE_OBJECT;
    obj->data.object.keys = NULL;
    obj->data.object.values = NULL;
    obj->data.object.count = 0;
    
    skip_whitespace(json);
    
    if (**json == '}') {
        (*json)++;
        return obj;
    }
    
    while (1) {
        skip_whitespace(json);
        
        /* Parse key */
        char *key = parse_string_raw(json);
        if (key == NULL) {
            free_value(obj);
            return NULL;
        }
        
        skip_whitespace(json);
        
        /* Expect colon */
        if (**json != ':') {
            free(key);
            free_value(obj);
            return NULL;
        }
        (*json)++;
        
        skip_whitespace(json);
        
        /* Parse value */
        json_value_t *val = parse_value(json);
        if (val == NULL) {
            free(key);
            free_value(obj);
            return NULL;
        }
        
        /* Add to object */
        int new_count = obj->data.object.count + 1;
        char **new_keys = (char**)realloc(obj->data.object.keys, new_count * sizeof(char*));
        json_value_t **new_values = (json_value_t**)realloc(obj->data.object.values, new_count * sizeof(json_value_t*));
        
        if (new_keys == NULL || new_values == NULL) {
            free(key);
            free_value(val);
            free_value(obj);
            return NULL;
        }
        
        obj->data.object.keys = new_keys;
        obj->data.object.values = new_values;
        obj->data.object.keys[obj->data.object.count] = key;
        obj->data.object.values[obj->data.object.count] = val;
        obj->data.object.count = new_count;
        
        skip_whitespace(json);
        
        if (**json == '}') {
            (*json)++;
            break;
        }
        
        if (**json != ',') {
            free_value(obj);
            return NULL;
        }
        (*json)++;
    }
    
    return obj;
}

static json_value_t* parse_array(const char **json)
{
    if (**json != '[') return NULL;
    (*json)++;
    
    json_value_t *arr = (json_value_t*)calloc(1, sizeof(json_value_t));
    if (arr == NULL) return NULL;
    
    arr->type = MIMI_CONFIG_TYPE_ARRAY;
    arr->data.array.items = NULL;
    arr->data.array.count = 0;
    
    skip_whitespace(json);
    
    if (**json == ']') {
        (*json)++;
        return arr;
    }
    
    while (1) {
        skip_whitespace(json);
        
        json_value_t *val = parse_value(json);
        if (val == NULL) {
            free_value(arr);
            return NULL;
        }
        
        int new_count = arr->data.array.count + 1;
        json_value_t **new_items = (json_value_t**)realloc(arr->data.array.items, new_count * sizeof(json_value_t*));
        
        if (new_items == NULL) {
            free_value(val);
            free_value(arr);
            return NULL;
        }
        
        arr->data.array.items = new_items;
        arr->data.array.items[arr->data.array.count] = val;
        arr->data.array.count = new_count;
        
        skip_whitespace(json);
        
        if (**json == ']') {
            (*json)++;
            break;
        }
        
        if (**json != ',') {
            free_value(arr);
            return NULL;
        }
        (*json)++;
    }
    
    return arr;
}

static json_value_t* parse_value(const char **json)
{
    skip_whitespace(json);
    
    if (**json == '"') {
        return parse_string(json);
    } else if (**json == '{') {
        return parse_object(json);
    } else if (**json == '[') {
        return parse_array(json);
    } else if (**json == 't' && strncmp(*json, "true", 4) == 0) {
        (*json) += 4;
        json_value_t *val = (json_value_t*)calloc(1, sizeof(json_value_t));
        if (val) {
            val->type = MIMI_CONFIG_TYPE_BOOLEAN;
            val->data.bool_val = 1;
        }
        return val;
    } else if (**json == 'f' && strncmp(*json, "false", 5) == 0) {
        (*json) += 5;
        json_value_t *val = (json_value_t*)calloc(1, sizeof(json_value_t));
        if (val) {
            val->type = MIMI_CONFIG_TYPE_BOOLEAN;
            val->data.bool_val = 0;
        }
        return val;
    } else if (**json == 'n' && strncmp(*json, "null", 4) == 0) {
        (*json) += 4;
        json_value_t *val = (json_value_t*)calloc(1, sizeof(json_value_t));
        if (val) {
            val->type = MIMI_CONFIG_TYPE_NULL;
        }
        return val;
    } else if (**json == '-' || (**json >= '0' && **json <= '9')) {
        return parse_number(json);
    }
    
    return NULL;
}

static void free_value(json_value_t *val)
{
    if (val == NULL) return;
    
    switch (val->type) {
        case MIMI_CONFIG_TYPE_STRING:
            free(val->data.string_val);
            break;
        case MIMI_CONFIG_TYPE_ARRAY:
            for (int i = 0; i < val->data.array.count; i++) {
                free_value(val->data.array.items[i]);
            }
            free(val->data.array.items);
            break;
        case MIMI_CONFIG_TYPE_OBJECT:
            for (int i = 0; i < val->data.object.count; i++) {
                free(val->data.object.keys[i]);
                free_value(val->data.object.values[i]);
            }
            free(val->data.object.keys);
            free(val->data.object.values);
            break;
        default:
            break;
    }
    
    free(val);
}

static json_value_t* find_value(json_value_t *obj, const char *key)
{
    if (obj == NULL || obj->type != MIMI_CONFIG_TYPE_OBJECT) {
        return NULL;
    }
    
    /* Handle dot notation */
    const char *dot = strchr(key, '.');
    if (dot != NULL) {
        size_t first_len = dot - key;
        char *first_key = (char*)malloc(first_len + 1);
        if (first_key == NULL) return NULL;
        
        memcpy(first_key, key, first_len);
        first_key[first_len] = '\0';
        
        json_value_t *nested = NULL;
        for (int i = 0; i < obj->data.object.count; i++) {
            if (strcmp(obj->data.object.keys[i], first_key) == 0) {
                nested = obj->data.object.values[i];
                break;
            }
        }
        
        free(first_key);
        
        if (nested == NULL) return NULL;
        return find_value(nested, dot + 1);
    }
    
    /* Simple key lookup */
    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            return obj->data.object.values[i];
        }
    }
    
    return NULL;
}

/* ============================================================================
 * File Utilities
 * ============================================================================ */

static char* read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    
    char *content = (char*)malloc(size + 1);
    if (content == NULL) {
        fclose(f);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, f);
    fclose(f);
    
    content[read_size] = '\0';
    return content;
}

static int64_t get_file_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (int64_t)st.st_mtime * 1000;
}

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

mimi_config_t* mimi_config_load(const char *path)
{
    if (path == NULL) {
        return NULL;
    }
    
    char *content = read_file(path);
    if (content == NULL) {
        return NULL;
    }
    
    mimi_config_t *cfg = (mimi_config_t*)calloc(1, sizeof(mimi_config_t));
    if (cfg == NULL) {
        free(content);
        return NULL;
    }
    
    cfg->path = strdup(path);
    if (cfg->path == NULL) {
        free(content);
        free(cfg);
        return NULL;
    }
    
    const char *json_ptr = content;
    cfg->root = parse_value(&json_ptr);
    free(content);
    
    if (cfg->root == NULL) {
        free(cfg->path);
        free(cfg);
        return NULL;
    }
    
    cfg->mtime = get_file_mtime(path);
    cfg->watch_fd = -1;
    cfg->watching = 0;
    
    return cfg;
}

mimi_config_t* mimi_config_load_json(const char *json)
{
    if (json == NULL) {
        return NULL;
    }
    
    mimi_config_t *cfg = (mimi_config_t*)calloc(1, sizeof(mimi_config_t));
    if (cfg == NULL) {
        return NULL;
    }
    
    const char *json_ptr = json;
    cfg->root = parse_value(&json_ptr);
    
    if (cfg->root == NULL) {
        free(cfg);
        return NULL;
    }
    
    cfg->path = NULL;
    cfg->mtime = 0;
    cfg->watch_fd = -1;
    cfg->watching = 0;
    
    return cfg;
}

void mimi_config_free(mimi_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    
    mimi_config_unwatch(cfg);
    free_value(cfg->root);
    free(cfg->path);
    free(cfg);
}

/* ============================================================================
 * Reading Configuration
 * ============================================================================ */

int mimi_config_get_string(mimi_config_t *cfg, const char *key, char **value)
{
    if (cfg == NULL || key == NULL || value == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    if (val == NULL) {
        return MIMI_CONFIG_ERR_NOT_FOUND;
    }
    
    if (val->type != MIMI_CONFIG_TYPE_STRING) {
        return MIMI_CONFIG_ERR_TYPE_MISMATCH;
    }
    
    *value = strdup(val->data.string_val);
    return (*value != NULL) ? MIMI_CONFIG_OK : MIMI_CONFIG_ERR_NO_MEMORY;
}

int mimi_config_get_int(mimi_config_t *cfg, const char *key, int *value)
{
    if (cfg == NULL || key == NULL || value == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    if (val == NULL) {
        return MIMI_CONFIG_ERR_NOT_FOUND;
    }
    
    if (val->type == MIMI_CONFIG_TYPE_INTEGER) {
        *value = val->data.int_val;
        return MIMI_CONFIG_OK;
    } else if (val->type == MIMI_CONFIG_TYPE_FLOAT) {
        *value = (int)val->data.float_val;
        return MIMI_CONFIG_OK;
    }
    
    return MIMI_CONFIG_ERR_TYPE_MISMATCH;
}

int mimi_config_get_float(mimi_config_t *cfg, const char *key, double *value)
{
    if (cfg == NULL || key == NULL || value == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    if (val == NULL) {
        return MIMI_CONFIG_ERR_NOT_FOUND;
    }
    
    if (val->type == MIMI_CONFIG_TYPE_FLOAT) {
        *value = val->data.float_val;
        return MIMI_CONFIG_OK;
    } else if (val->type == MIMI_CONFIG_TYPE_INTEGER) {
        *value = (double)val->data.int_val;
        return MIMI_CONFIG_OK;
    }
    
    return MIMI_CONFIG_ERR_TYPE_MISMATCH;
}

int mimi_config_get_bool(mimi_config_t *cfg, const char *key, int *value)
{
    if (cfg == NULL || key == NULL || value == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    if (val == NULL) {
        return MIMI_CONFIG_ERR_NOT_FOUND;
    }
    
    if (val->type == MIMI_CONFIG_TYPE_BOOLEAN) {
        *value = val->data.bool_val;
        return MIMI_CONFIG_OK;
    } else if (val->type == MIMI_CONFIG_TYPE_INTEGER) {
        *value = (val->data.int_val != 0) ? 1 : 0;
        return MIMI_CONFIG_OK;
    } else if (val->type == MIMI_CONFIG_TYPE_STRING) {
        *value = (strcmp(val->data.string_val, "true") == 0 ||
                  strcmp(val->data.string_val, "1") == 0) ? 1 : 0;
        return MIMI_CONFIG_OK;
    }
    
    return MIMI_CONFIG_ERR_TYPE_MISMATCH;
}

int mimi_config_has_key(mimi_config_t *cfg, const char *key)
{
    if (cfg == NULL || key == NULL) {
        return -1;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    return (val != NULL) ? 1 : 0;
}

mimi_config_type_t mimi_config_get_type(mimi_config_t *cfg, const char *key)
{
    if (cfg == NULL || key == NULL) {
        return MIMI_CONFIG_TYPE_NULL;
    }
    
    json_value_t *val = find_value(cfg->root, key);
    return (val != NULL) ? val->type : MIMI_CONFIG_TYPE_NULL;
}

/* ============================================================================
 * Hot Reload
 * ============================================================================ */

int mimi_config_watch(mimi_config_t *cfg, const mimi_config_options_t *options)
{
    if (cfg == NULL || cfg->path == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    if (options != NULL) {
        memcpy(&cfg->watch_options, options, sizeof(mimi_config_options_t));
    } else {
        memset(&cfg->watch_options, 0, sizeof(mimi_config_options_t));
        cfg->watch_options.watch_interval_ms = 1000;
    }
    
#ifdef __linux__
    cfg->watch_fd = inotify_init1(IN_NONBLOCK);
    if (cfg->watch_fd < 0) {
        cfg->watch_fd = -1;
    } else {
        int wd = inotify_add_watch(cfg->watch_fd, cfg->path, IN_MODIFY);
        if (wd < 0) {
            close(cfg->watch_fd);
            cfg->watch_fd = -1;
        }
    }
#endif
    
    cfg->watching = 1;
    return MIMI_CONFIG_OK;
}

int mimi_config_reload(mimi_config_t *cfg)
{
    if (cfg == NULL || cfg->path == NULL) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    int64_t new_mtime = get_file_mtime(cfg->path);
    if (new_mtime == cfg->mtime) {
        return MIMI_CONFIG_OK; /* No change */
    }
    
    char *content = read_file(cfg->path);
    if (content == NULL) {
        return MIMI_CONFIG_ERR_IO;
    }
    
    const char *json_ptr = content;
    json_value_t *new_root = parse_value(&json_ptr);
    free(content);
    
    if (new_root == NULL) {
        return MIMI_CONFIG_ERR_PARSE_ERROR;
    }
    
    /* Replace old root */
    free_value(cfg->root);
    cfg->root = new_root;
    cfg->mtime = new_mtime;
    
    return MIMI_CONFIG_OK;
}

void mimi_config_unwatch(mimi_config_t *cfg)
{
    if (cfg == NULL || !cfg->watching) {
        return;
    }
    
#ifdef __linux__
    if (cfg->watch_fd >= 0) {
        close(cfg->watch_fd);
        cfg->watch_fd = -1;
    }
#endif
    
    cfg->watching = 0;
}

/* ============================================================================
 * Validation
 * ============================================================================ */

int mimi_config_validate(mimi_config_t *cfg, const char **required_keys,
                         int key_count, char **error_msg)
{
    if (cfg == NULL || required_keys == NULL || key_count <= 0) {
        return MIMI_CONFIG_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < key_count; i++) {
        if (!mimi_config_has_key(cfg, required_keys[i])) {
            if (error_msg != NULL) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Missing required key: %s", required_keys[i]);
                *error_msg = strdup(msg);
            }
            return MIMI_CONFIG_ERR_VALIDATION;
        }
    }
    
    return MIMI_CONFIG_OK;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* mimi_config_version(void)
{
    return MIMI_CONFIG_VERSION;
}

const char* mimi_config_strerror(int error)
{
    switch (error) {
        case MIMI_CONFIG_OK:
            return "Success";
        case MIMI_CONFIG_ERR_INVALID_ARG:
            return "Invalid argument";
        case MIMI_CONFIG_ERR_NO_MEMORY:
            return "Memory allocation failed";
        case MIMI_CONFIG_ERR_FILE_NOT_FOUND:
            return "Configuration file not found";
        case MIMI_CONFIG_ERR_PARSE_ERROR:
            return "JSON parse error";
        case MIMI_CONFIG_ERR_TYPE_MISMATCH:
            return "Type mismatch";
        case MIMI_CONFIG_ERR_NOT_FOUND:
            return "Key not found";
        case MIMI_CONFIG_ERR_VALIDATION:
            return "Validation failed";
        case MIMI_CONFIG_ERR_IO:
            return "I/O error";
        default:
            return "Unknown error";
    }
}

const char* mimi_config_get_path(mimi_config_t *cfg)
{
    return (cfg != NULL) ? cfg->path : NULL;
}

int64_t mimi_config_get_mtime(mimi_config_t *cfg)
{
    return (cfg != NULL) ? cfg->mtime : 0;
}
