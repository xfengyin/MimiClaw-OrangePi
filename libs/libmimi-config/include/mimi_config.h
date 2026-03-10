/**
 * @file mimi_config.h
 * @brief MimiClaw Config Library - Configuration Parsing & Hot Reload
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 */

#ifndef MIMI_CONFIG_H
#define MIMI_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Config library version string
 */
#define MIMI_CONFIG_VERSION "2.0.0"

/**
 * @brief Maximum configuration path length
 */
#define MIMI_CONFIG_MAX_PATH_LEN 512

/**
 * @brief Maximum key length
 */
#define MIMI_CONFIG_MAX_KEY_LEN 256

/**
 * @brief Error codes
 */
typedef enum {
    MIMI_CONFIG_OK = 0,              /**< Success */
    MIMI_CONFIG_ERR_INVALID_ARG = -1,     /**< Invalid argument */
    MIMI_CONFIG_ERR_NO_MEMORY = -2,       /**< Memory allocation failed */
    MIMI_CONFIG_ERR_FILE_NOT_FOUND = -3,  /**< Configuration file not found */
    MIMI_CONFIG_ERR_PARSE_ERROR = -4,     /**< JSON/YAML parse error */
    MIMI_CONFIG_ERR_TYPE_MISMATCH = -5,   /**< Type mismatch */
    MIMI_CONFIG_ERR_NOT_FOUND = -6,       /**< Key not found */
    MIMI_CONFIG_ERR_VALIDATION = -7,      /**< Validation failed */
    MIMI_CONFIG_ERR_IO = -8               /**< I/O error */
} mimi_config_error_t;

/**
 * @brief Configuration value type
 */
typedef enum {
    MIMI_CONFIG_TYPE_NULL,
    MIMI_CONFIG_TYPE_STRING,
    MIMI_CONFIG_TYPE_INTEGER,
    MIMI_CONFIG_TYPE_FLOAT,
    MIMI_CONFIG_TYPE_BOOLEAN,
    MIMI_CONFIG_TYPE_ARRAY,
    MIMI_CONFIG_TYPE_OBJECT
} mimi_config_type_t;

/**
 * @brief Reload callback function type
 * 
 * @param user_data User data passed to watch function
 * @param status Reload status (0=success, negative=error)
 */
typedef void (*mimi_config_reload_cb)(void *user_data, int status);

/**
 * @brief Configuration watch options
 */
typedef struct {
    int watch_enabled;                          /**< Enable file watching */
    mimi_config_reload_cb on_reload;            /**< Reload callback */
    void *user_data;                            /**< User data for callback */
    int watch_interval_ms;                      /**< Polling interval in ms (default: 1000) */
} mimi_config_options_t;

/**
 * @brief Opaque configuration structure
 * 
 * Contains parsed configuration data and watch state.
 */
typedef struct mimi_config mimi_config_t;

/**
 * @brief Opaque configuration value structure
 */
typedef struct mimi_config_value mimi_config_value_t;

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

/**
 * @brief Load configuration from a file
 * 
 * Supports JSON format. Parses the file and creates a configuration object.
 * 
 * @param path Path to configuration file
 * @return Configuration handle, or NULL on failure
 * 
 * @note Memory: Caller must call mimi_config_free() when done
 * @note Thread-safe: Read operations are thread-safe
 */
mimi_config_t* mimi_config_load(const char *path);

/**
 * @brief Load configuration from a JSON string
 * 
 * @param json JSON string to parse
 * @return Configuration handle, or NULL on failure
 * 
 * @note Memory: Caller must call mimi_config_free() when done
 */
mimi_config_t* mimi_config_load_json(const char *json);

/**
 * @brief Free a configuration object
 * 
 * @param cfg Configuration to free (can be NULL)
 */
void mimi_config_free(mimi_config_t *cfg);

/* ============================================================================
 * Reading Configuration
 * ============================================================================ */

/**
 * @brief Get a string value
 * 
 * @param cfg Configuration object
 * @param key Key to retrieve (dot notation for nested: "database.host")
 * @param value Pointer to receive the value (allocated, must be freed)
 * @return MIMI_CONFIG_OK on success, error code on failure
 * 
 * @note Memory: Caller must free *value when done
 */
int mimi_config_get_string(mimi_config_t *cfg, const char *key, char **value);

/**
 * @brief Get an integer value
 * 
 * @param cfg Configuration object
 * @param key Key to retrieve
 * @param value Pointer to receive the value
 * @return MIMI_CONFIG_OK on success, error code on failure
 */
int mimi_config_get_int(mimi_config_t *cfg, const char *key, int *value);

/**
 * @brief Get a float/double value
 * 
 * @param cfg Configuration object
 * @param key Key to retrieve
 * @param value Pointer to receive the value
 * @return MIMI_CONFIG_OK on success, error code on failure
 */
int mimi_config_get_float(mimi_config_t *cfg, const char *key, double *value);

/**
 * @brief Get a boolean value
 * 
 * Accepts: true/false, 1/0, "true"/"false"
 * 
 * @param cfg Configuration object
 * @param key Key to retrieve
 * @param value Pointer to receive the value (1=true, 0=false)
 * @return MIMI_CONFIG_OK on success, error code on failure
 */
int mimi_config_get_bool(mimi_config_t *cfg, const char *key, int *value);

/**
 * @brief Check if a key exists
 * 
 * @param cfg Configuration object
 * @param key Key to check
 * @return 1 if exists, 0 if not, negative on error
 */
int mimi_config_has_key(mimi_config_t *cfg, const char *key);

/**
 * @brief Get the type of a value
 * 
 * @param cfg Configuration object
 * @param key Key to check
 * @return Value type, or MIMI_CONFIG_TYPE_NULL on error
 */
mimi_config_type_t mimi_config_get_type(mimi_config_t *cfg, const char *key);

/* ============================================================================
 * Hot Reload
 * ============================================================================ */

/**
 * @brief Enable configuration file watching
 * 
 * Monitors the configuration file for changes and automatically reloads.
 * On Linux, uses inotify if available, otherwise falls back to polling.
 * 
 * @param cfg Configuration object
 * @param options Watch options
 * @return MIMI_CONFIG_OK on success, error code on failure
 */
int mimi_config_watch(mimi_config_t *cfg, const mimi_config_options_t *options);

/**
 * @brief Manually reload configuration from file
 * 
 * @param cfg Configuration object
 * @return MIMI_CONFIG_OK on success, error code on failure
 */
int mimi_config_reload(mimi_config_t *cfg);

/**
 * @brief Stop watching for changes
 * 
 * @param cfg Configuration object
 */
void mimi_config_unwatch(mimi_config_t *cfg);

/* ============================================================================
 * Validation
 * ============================================================================ */

/**
 * @brief Validate configuration against required keys
 * 
 * @param cfg Configuration object
 * @param required_keys Array of required key names
 * @param key_count Number of required keys
 * @param error_msg Pointer to receive error message (allocated, must be freed)
 * @return MIMI_CONFIG_OK on success, MIMI_CONFIG_ERR_VALIDATION on failure
 * 
 * @note Memory: Caller must free *error_msg if validation fails
 */
int mimi_config_validate(mimi_config_t *cfg, const char **required_keys,
                         int key_count, char **error_msg);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get the library version string
 * 
 * @return Version string (statically allocated, do not free)
 */
const char* mimi_config_version(void);

/**
 * @brief Get error message for an error code
 * 
 * @param error Error code
 * @return Error message string (statically allocated, do not free)
 */
const char* mimi_config_strerror(int error);

/**
 * @brief Get the configuration file path
 * 
 * @param cfg Configuration object
 * @return File path (statically allocated within cfg, do not free)
 */
const char* mimi_config_get_path(mimi_config_t *cfg);

/**
 * @brief Get the last modification time of the config file
 * 
 * @param cfg Configuration object
 * @return Modification time as Unix timestamp (milliseconds), or 0 on error
 */
int64_t mimi_config_get_mtime(mimi_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_CONFIG_H */
