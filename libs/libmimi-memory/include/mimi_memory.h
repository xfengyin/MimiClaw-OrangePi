/**
 * @file mimi_memory.h
 * @brief MimiClaw Memory Library - SQLite Persistence & Session Management
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 */

#ifndef MIMI_MEMORY_H
#define MIMI_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* 共享库符号可见性 */
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef MIMI_MEMORY_BUILDING_DLL
        #define MIMI_MEMORY_API __declspec(dllexport)
    #else
        #define MIMI_MEMORY_API __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define MIMI_MEMORY_API __attribute__((visibility("default")))
    #else
        #define MIMI_MEMORY_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory library version string
 */
#define MIMI_MEMORY_VERSION "2.0.0"

/**
 * @brief Maximum database path length
 */
#define MIMI_MEMORY_MAX_PATH_LEN 512

/**
 * @brief Maximum session ID length
 */
#define MIMI_MEMORY_MAX_SESSION_ID_LEN 128

/**
 * @brief Maximum key length for memory entries
 */
#define MIMI_MEMORY_MAX_KEY_LEN 256

/**
 * @brief Error codes
 */
typedef enum {
    MIMI_MEMORY_OK = 0,              /**< Success */
    MIMI_MEMORY_ERR_INVALID_ARG = -1,     /**< Invalid argument */
    MIMI_MEMORY_ERR_NO_MEMORY = -2,       /**< Memory allocation failed */
    MIMI_MEMORY_ERR_DB_ERROR = -3,        /**< SQLite database error */
    MIMI_MEMORY_ERR_NOT_FOUND = -4,       /**< Resource not found */
    MIMI_MEMORY_ERR_EXISTS = -5,          /**< Resource already exists */
    MIMI_MEMORY_ERR_POOL_FULL = -6,       /**< Connection pool is full */
    MIMI_MEMORY_ERR_INIT_FAILED = -7      /**< Initialization failed */
} mimi_memory_error_t;

/**
 * @brief Memory pool configuration
 * 
 * Configuration for creating a memory pool with connection pooling.
 */
typedef struct {
    char db_path[MIMI_MEMORY_MAX_PATH_LEN];  /**< SQLite database file path */
    int pool_size;                            /**< Connection pool size (1-16) */
    int max_idle_time;                        /**< Max idle time in seconds */
    int enable_wal;                           /**< Enable WAL mode (recommended) */
} mimi_mem_config_t;

/**
 * @brief Opaque memory pool structure
 * 
 * Manages SQLite connections with pooling for efficient reuse.
 */
typedef struct mimi_mem_pool mimi_mem_pool_t;

/**
 * @brief Message structure for conversation history
 */
typedef struct {
    int64_t msg_id;              /**< Unique message ID */
    char *session_id;            /**< Session identifier */
    char *role;                  /**< Message role: "user", "assistant", "system" */
    char *content;               /**< Message content */
    int64_t timestamp;           /**< Unix timestamp (milliseconds) */
} mimi_mem_message_t;

/**
 * @brief Memory entry structure
 */
typedef struct {
    char *key;                   /**< Memory key */
    char *value;                 /**< Memory value */
    int64_t created_at;          /**< Creation timestamp */
    int64_t updated_at;          /**< Last update timestamp */
} mimi_mem_entry_t;

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

/**
 * @brief Create a memory pool
 * 
 * Initializes SQLite database with connection pooling.
 * Creates database schema if it doesn't exist.
 * 
 * @param config Configuration structure
 * @return Memory pool handle, or NULL on failure
 * 
 * @note Thread-safe: Pool can be used concurrently from multiple threads
 * @note Memory: Caller must call mimi_mem_pool_destroy() when done
 */
mimi_mem_pool_t* mimi_mem_pool_create(const mimi_mem_config_t *config);

/**
 * @brief Destroy a memory pool
 * 
 * Closes all database connections and frees resources.
 * 
 * @param pool Memory pool to destroy (can be NULL)
 */
void mimi_mem_pool_destroy(mimi_mem_pool_t *pool);

/* ============================================================================
 * Session Management
 * ============================================================================ */

/**
 * @brief Create a new session
 * 
 * @param pool Memory pool
 * @param session_id Session identifier to create
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_session_create(mimi_mem_pool_t *pool, const char *session_id);

/**
 * @brief Delete a session and all associated data
 * 
 * @param pool Memory pool
 * @param session_id Session identifier to delete
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_session_delete(mimi_mem_pool_t *pool, const char *session_id);

/**
 * @brief List all sessions
 * 
 * @param pool Memory pool
 * @param sessions Pointer to receive array of session IDs
 * @param count Pointer to receive number of sessions
 * @return MIMI_MEMORY_OK on success, error code on failure
 * 
 * @note Memory: Caller must free each session ID and the sessions array
 */
int mimi_mem_session_list(mimi_mem_pool_t *pool, char ***sessions, int *count);

/**
 * @brief Check if a session exists
 * 
 * @param pool Memory pool
 * @param session_id Session identifier to check
 * @return 1 if exists, 0 if not, negative on error
 */
int mimi_mem_session_exists(mimi_mem_pool_t *pool, const char *session_id);

/* ============================================================================
 * Message Storage
 * ============================================================================ */

/**
 * @brief Append a message to session history
 * 
 * @param pool Memory pool
 * @param session_id Session identifier
 * @param role Message role ("user", "assistant", "system")
 * @param content Message content
 * @param msg_id Pointer to receive the generated message ID (can be NULL)
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_message_append(mimi_mem_pool_t *pool, const char *session_id,
                            const char *role, const char *content, int64_t *msg_id);

/**
 * @brief Query messages from a session
 * 
 * @param pool Memory pool
 * @param session_id Session identifier
 * @param limit Maximum number of messages to retrieve (0 = all)
 * @param messages Pointer to receive array of messages
 * @param count Pointer to receive number of messages
 * @return MIMI_MEMORY_OK on success, error code on failure
 * 
 * @note Memory: Caller must free each message and the messages array
 */
int mimi_mem_message_query(mimi_mem_pool_t *pool, const char *session_id,
                           int limit, mimi_mem_message_t ***messages, int *count);

/**
 * @brief Free a message structure
 * 
 * @param msg Message to free
 */
void mimi_mem_message_free(mimi_mem_message_t *msg);

/**
 * @brief Delete messages from a session
 * 
 * @param pool Memory pool
 * @param session_id Session identifier
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_message_clear(mimi_mem_pool_t *pool, const char *session_id);

/**
 * @brief Batch append messages (transaction-optimized)
 * 
 * Inserts multiple messages in a single transaction for better performance.
 * 
 * @param pool Memory pool
 * @param session_id Session identifier
 * @param roles Array of message roles
 * @param contents Array of message contents
 * @param count Number of messages
 * @return MIMI_MEMORY_OK on success, error code on failure
 * 
 * @note All messages are inserted atomically (all or nothing)
 */
int mimi_mem_message_batch_append(mimi_mem_pool_t *pool, const char *session_id,
                                  const char **roles, const char **contents, int count);

/* ============================================================================
 * Memory Operations (Key-Value Store)
 * ============================================================================ */

/**
 * @brief Write a memory entry
 * 
 * Creates or updates a key-value pair.
 * 
 * @param pool Memory pool
 * @param key Memory key
 * @param value Memory value
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_write(mimi_mem_pool_t *pool, const char *key, const char *value);

/**
 * @brief Read a memory entry
 * 
 * @param pool Memory pool
 * @param key Memory key to retrieve
 * @param value Pointer to receive the value (allocated, must be freed)
 * @return MIMI_MEMORY_OK on success, MIMI_MEMORY_ERR_NOT_FOUND if key doesn't exist
 * 
 * @note Memory: Caller must free *value when done
 */
int mimi_mem_read(mimi_mem_pool_t *pool, const char *key, char **value);

/**
 * @brief Delete a memory entry
 * 
 * @param pool Memory pool
 * @param key Memory key to delete
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_delete(mimi_mem_pool_t *pool, const char *key);

/**
 * @brief Search memory entries by query
 * 
 * Performs a simple text search in keys and values.
 * 
 * @param pool Memory pool
 * @param query Search query string
 * @param results Pointer to receive array of matching entries
 * @param count Pointer to receive number of results
 * @return MIMI_MEMORY_OK on success, error code on failure
 * 
 * @note Memory: Caller must free each entry and the results array
 */
int mimi_mem_search(mimi_mem_pool_t *pool, const char *query,
                    mimi_mem_entry_t ***results, int *count);

/**
 * @brief Free a memory entry structure
 * 
 * @param entry Entry to free
 */
void mimi_mem_entry_free(mimi_mem_entry_t *entry);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get the library version string
 * 
 * @return Version string (statically allocated, do not free)
 */
const char* mimi_mem_version(void);

/**
 * @brief Get error message for an error code
 * 
 * @param error Error code
 * @return Error message string (statically allocated, do not free)
 */
const char* mimi_mem_strerror(int error);

/**
 * @brief Get last SQLite error message
 * 
 * @param pool Memory pool
 * @return SQLite error message (valid until next operation)
 */
const char* mimi_mem_last_error(mimi_mem_pool_t *pool);

/**
 * @brief Run database vacuum to optimize file size
 * 
 * @param pool Memory pool
 * @return MIMI_MEMORY_OK on success, error code on failure
 */
int mimi_mem_vacuum(mimi_mem_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_MEMORY_H */
