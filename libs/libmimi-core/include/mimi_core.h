/**
 * @file mimi_core.h
 * @brief MimiClaw Core Library - AI Agent Loop & Context Management
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 */

#ifndef MIMI_CORE_H
#define MIMI_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Core library version string
 */
#define MIMI_CORE_VERSION "2.0.0"

/**
 * @brief Maximum API key length
 */
#define MIMI_CORE_MAX_API_KEY_LEN 256

/**
 * @brief Maximum model name length
 */
#define MIMI_CORE_MAX_MODEL_LEN 64

/**
 * @brief Maximum session ID length
 */
#define MIMI_CORE_MAX_SESSION_ID_LEN 128

/**
 * @brief Maximum context key length
 */
#define MIMI_CORE_MAX_CONTEXT_KEY_LEN 256

/**
 * @brief Error codes
 */
typedef enum {
    MIMI_CORE_OK = 0,           /**< Success */
    MIMI_CORE_ERR_INVALID_ARG = -1,  /**< Invalid argument */
    MIMI_CORE_ERR_NO_MEMORY = -2,    /**< Memory allocation failed */
    MIMI_CORE_ERR_INIT_FAILED = -3,  /**< Initialization failed */
    MIMI_CORE_ERR_NOT_FOUND = -4,    /**< Resource not found */
    MIMI_CORE_ERR_API_ERROR = -5,    /**< External API error */
    MIMI_CORE_ERR_TIMEOUT = -6,      /**< Operation timeout */
    MIMI_CORE_ERR_SESSION_EXISTS = -7, /**< Session already exists */
    MIMI_CORE_ERR_SESSION_NOT_FOUND = -8 /**< Session not found */
} mimi_core_error_t;

/**
 * @brief Core configuration structure
 * 
 * Configuration for initializing the MimiCore context.
 * All fields must be set before calling mimi_core_init().
 */
typedef struct {
    char api_key[MIMI_CORE_MAX_API_KEY_LEN];   /**< API key for AI service */
    char model[MIMI_CORE_MAX_MODEL_LEN];       /**< Model name (e.g., "claude-3-5-sonnet") */
    int max_tokens;                             /**< Maximum tokens in response */
    float temperature;                          /**< Sampling temperature (0.0-2.0) */
    int timeout_ms;                             /**< API request timeout in milliseconds */
} mimi_core_config_t;

/**
 * @brief Opaque core context structure
 * 
 * Contains all state for a MimiCore instance.
 * No global state - supports multiple concurrent instances.
 */
typedef struct mimi_core_ctx mimi_core_ctx_t;

/**
 * @brief Message role enumeration
 */
typedef enum {
    MIMI_CORE_ROLE_USER,      /**< User message */
    MIMI_CORE_ROLE_ASSISTANT, /**< Assistant response */
    MIMI_CORE_ROLE_SYSTEM     /**< System instruction */
} mimi_core_role_t;

/**
 * @brief Tool call result structure
 */
typedef struct {
    char *tool_name;          /**< Name of the called tool */
    char *input;              /**< Input provided to the tool */
    char *output;             /**< Output from the tool */
    int status;               /**< Execution status (0=success) */
} mimi_core_tool_result_t;

/**
 * @brief Chat response structure
 */
typedef struct {
    char *content;            /**< Response text content */
    char *finish_reason;      /**< Reason for finishing (e.g., "stop", "tool_calls") */
    int usage_prompt_tokens;  /**< Number of prompt tokens used */
    int usage_completion_tokens; /**< Number of completion tokens used */
    mimi_core_tool_result_t *tool_results; /**< Tool call results (if any) */
    int tool_results_count;   /**< Number of tool results */
} mimi_core_response_t;

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

/**
 * @brief Initialize a MimiCore context
 * 
 * Allocates and initializes a new MimiCore context with the provided configuration.
 * The context must be destroyed with mimi_core_destroy() when no longer needed.
 * 
 * @param ctx Pointer to context pointer to be initialized
 * @param config Configuration structure (must be valid)
 * @return MIMI_CORE_OK on success, error code on failure
 * 
 * @note Thread-safe: Each context is independent and can be used in separate threads
 * @note Memory: Caller must call mimi_core_destroy() to free resources
 */
int mimi_core_init(mimi_core_ctx_t **ctx, const mimi_core_config_t *config);

/**
 * @brief Destroy a MimiCore context
 * 
 * Frees all resources associated with the context.
 * After this call, the context pointer is invalid.
 * 
 * @param ctx Context to destroy (can be NULL)
 * @return MIMI_CORE_OK on success, error code on failure
 */
int mimi_core_destroy(mimi_core_ctx_t *ctx);

/* ============================================================================
 * Chat & Conversation
 * ============================================================================ */

/**
 * @brief Process a user message and generate a response
 * 
 * Sends the user message to the AI model and returns the response.
 * Handles tool calls automatically if the model requests them.
 * 
 * @param ctx Core context (must be initialized)
 * @param session_id Unique session identifier for conversation history
 * @param user_message User's input message
 * @param response Pointer to receive the response structure
 * @return MIMI_CORE_OK on success, error code on failure
 * 
 * @note Memory: Caller must free response->content and response->finish_reason
 * @note Session: Creates session automatically if it doesn't exist
 */
int mimi_core_chat(mimi_core_ctx_t *ctx, const char *session_id,
                   const char *user_message, mimi_core_response_t *response);

/**
 * @brief Free a chat response structure
 * 
 * @param response Response structure to free
 */
void mimi_core_response_free(mimi_core_response_t *response);

/* ============================================================================
 * Context Management
 * ============================================================================ */

/**
 * @brief Set a context value for a session
 * 
 * Stores a key-value pair in the session context.
 * Context is persisted across chat calls within the same session.
 * 
 * @param ctx Core context
 * @param session_id Session identifier
 * @param key Context key (max MIMI_CORE_MAX_CONTEXT_KEY_LEN)
 * @param value Context value (string)
 * @return MIMI_CORE_OK on success, error code on failure
 */
int mimi_core_set_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, const char *value);

/**
 * @brief Get a context value from a session
 * 
 * Retrieves a previously stored context value.
 * 
 * @param ctx Core context
 * @param session_id Session identifier
 * @param key Context key to retrieve
 * @param value Pointer to receive the value (allocated, must be freed)
 * @return MIMI_CORE_OK on success, MIMI_CORE_ERR_NOT_FOUND if key doesn't exist
 * 
 * @note Memory: Caller must free *value when done
 */
int mimi_core_get_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, char **value);

/**
 * @brief Delete a context value from a session
 * 
 * @param ctx Core context
 * @param session_id Session identifier
 * @param key Context key to delete
 * @return MIMI_CORE_OK on success, error code on failure
 */
int mimi_core_delete_context(mimi_core_ctx_t *ctx, const char *session_id,
                             const char *key);

/**
 * @brief Clear all context for a session
 * 
 * @param ctx Core context
 * @param session_id Session identifier
 * @return MIMI_CORE_OK on success, error code on failure
 */
int mimi_core_clear_context(mimi_core_ctx_t *ctx, const char *session_id);

/* ============================================================================
 * Session Management
 * ============================================================================ */

/**
 * @brief Create a new session
 * 
 * Creates an empty session with the given ID.
 * 
 * @param ctx Core context
 * @param session_id Session identifier to create
 * @return MIMI_CORE_OK on success, MIMI_CORE_ERR_SESSION_EXISTS if already exists
 */
int mimi_core_session_create(mimi_core_ctx_t *ctx, const char *session_id);

/**
 * @brief Delete a session
 * 
 * Deletes a session and all associated context/history.
 * 
 * @param ctx Core context
 * @param session_id Session identifier to delete
 * @return MIMI_CORE_OK on success, MIMI_CORE_ERR_SESSION_NOT_FOUND if not exists
 */
int mimi_core_session_delete(mimi_core_ctx_t *ctx, const char *session_id);

/**
 * @brief List all active sessions
 * 
 * @param ctx Core context
 * @param sessions Pointer to receive array of session IDs
 * @param count Pointer to receive number of sessions
 * @return MIMI_CORE_OK on success, error code on failure
 * 
 * @note Memory: Caller must free each session ID and the sessions array
 */
int mimi_core_session_list(mimi_core_ctx_t *ctx, char ***sessions, int *count);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get the library version string
 * 
 * @return Version string (statically allocated, do not free)
 */
const char* mimi_core_version(void);

/**
 * @brief Get error message for an error code
 * 
 * @param error Error code
 * @return Error message string (statically allocated, do not free)
 */
const char* mimi_core_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_CORE_H */
