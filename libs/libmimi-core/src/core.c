/**
 * @file core.c
 * @brief MimiClaw Core Library Implementation
 * @version 2.0.0
 */

#define _GNU_SOURCE

#include "mimi_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

/**
 * @brief Context entry for session context storage
 */
typedef struct mimi_core_context_entry {
    char *key;
    char *value;
    struct mimi_core_context_entry *next;
} mimi_core_context_entry_t;

/**
 * @brief Session structure
 */
typedef struct mimi_core_session {
    char *session_id;
    mimi_core_context_entry_t *context;
    struct mimi_core_session *next;
} mimi_core_session_t;

/**
 * @brief Internal core context structure
 */
struct mimi_core_ctx {
    mimi_core_config_t config;
    mimi_core_session_t *sessions;
    int initialized;
};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static int validate_config(const mimi_core_config_t *config)
{
    if (config == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    if (config->api_key[0] == '\0') {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    if (config->model[0] == '\0') {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    if (config->max_tokens <= 0) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    if (config->temperature < 0.0f || config->temperature > 2.0f) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    return MIMI_CORE_OK;
}

static mimi_core_session_t* find_session(mimi_core_ctx_t *ctx, const char *session_id)
{
    if (ctx == NULL || session_id == NULL) {
        return NULL;
    }
    
    mimi_core_session_t *session = ctx->sessions;
    while (session != NULL) {
        if (strcmp(session->session_id, session_id) == 0) {
            return session;
        }
        session = session->next;
    }
    return NULL;
}

static mimi_core_session_t* create_session(const char *session_id)
{
    mimi_core_session_t *session = (mimi_core_session_t*)malloc(sizeof(mimi_core_session_t));
    if (session == NULL) {
        return NULL;
    }
    
    session->session_id = strdup(session_id);
    if (session->session_id == NULL) {
        free(session);
        return NULL;
    }
    
    session->context = NULL;
    session->next = NULL;
    return session;
}

static void free_session(mimi_core_session_t *session)
{
    if (session == NULL) {
        return;
    }
    
    /* Free context entries */
    mimi_core_context_entry_t *entry = session->context;
    while (entry != NULL) {
        mimi_core_context_entry_t *next = entry->next;
        free(entry->key);
        free(entry->value);
        free(entry);
        entry = next;
    }
    
    free(session->session_id);
    free(session);
}

static int set_context_entry(mimi_core_session_t *session, const char *key, const char *value)
{
    if (session == NULL || key == NULL || value == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    /* Check if key already exists */
    mimi_core_context_entry_t *entry = session->context;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            char *new_value = strdup(value);
            if (new_value == NULL) {
                return MIMI_CORE_ERR_NO_MEMORY;
            }
            free(entry->value);
            entry->value = new_value;
            return MIMI_CORE_OK;
        }
        entry = entry->next;
    }
    
    /* Create new entry */
    entry = (mimi_core_context_entry_t*)malloc(sizeof(mimi_core_context_entry_t));
    if (entry == NULL) {
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    entry->key = strdup(key);
    entry->value = strdup(value);
    if (entry->key == NULL || entry->value == NULL) {
        free(entry->key);
        free(entry->value);
        free(entry);
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    entry->next = session->context;
    session->context = entry;
    return MIMI_CORE_OK;
}

static const char* get_context_entry(mimi_core_session_t *session, const char *key)
{
    if (session == NULL || key == NULL) {
        return NULL;
    }
    
    mimi_core_context_entry_t *entry = session->context;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

int mimi_core_init(mimi_core_ctx_t **ctx, const mimi_core_config_t *config)
{
    if (ctx == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    int ret = validate_config(config);
    if (ret != MIMI_CORE_OK) {
        return ret;
    }
    
    *ctx = (mimi_core_ctx_t*)calloc(1, sizeof(mimi_core_ctx_t));
    if (*ctx == NULL) {
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    memcpy(&(*ctx)->config, config, sizeof(mimi_core_config_t));
    (*ctx)->sessions = NULL;
    (*ctx)->initialized = 1;
    
    return MIMI_CORE_OK;
}

int mimi_core_destroy(mimi_core_ctx_t *ctx)
{
    if (ctx == NULL) {
        return MIMI_CORE_OK;
    }
    
    /* Free all sessions */
    mimi_core_session_t *session = ctx->sessions;
    while (session != NULL) {
        mimi_core_session_t *next = session->next;
        free_session(session);
        session = next;
    }
    
    ctx->initialized = 0;
    free(ctx);
    return MIMI_CORE_OK;
}

/* ============================================================================
 * Chat & Conversation
 * ============================================================================ */

int mimi_core_chat(mimi_core_ctx_t *ctx, const char *session_id,
                   const char *user_message, mimi_core_response_t *response)
{
    if (ctx == NULL || session_id == NULL || user_message == NULL || response == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    if (!ctx->initialized) {
        return MIMI_CORE_ERR_INIT_FAILED;
    }
    
    /* Auto-create session if not exists */
    mimi_core_session_t *session = find_session(ctx, session_id);
    if (session == NULL) {
        int ret = mimi_core_session_create(ctx, session_id);
        if (ret != MIMI_CORE_OK) {
            return ret;
        }
        session = find_session(ctx, session_id);
        if (session == NULL) {
            return MIMI_CORE_ERR_INIT_FAILED;
        }
    }
    
    /* Initialize response */
    memset(response, 0, sizeof(mimi_core_response_t));
    
    /* 
     * TODO: Implement actual AI API call
     * For now, return a placeholder response
     */
    response->content = strdup("Echo: Your message was received.");
    response->finish_reason = strdup("stop");
    if (response->content == NULL || response->finish_reason == NULL) {
        free(response->content);
        free(response->finish_reason);
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    response->usage_prompt_tokens = 10;
    response->usage_completion_tokens = 5;
    response->tool_results = NULL;
    response->tool_results_count = 0;
    
    return MIMI_CORE_OK;
}

void mimi_core_response_free(mimi_core_response_t *response)
{
    if (response == NULL) {
        return;
    }
    
    free(response->content);
    free(response->finish_reason);
    
    if (response->tool_results != NULL) {
        for (int i = 0; i < response->tool_results_count; i++) {
            free(response->tool_results[i].tool_name);
            free(response->tool_results[i].input);
            free(response->tool_results[i].output);
        }
        free(response->tool_results);
    }
    
    memset(response, 0, sizeof(mimi_core_response_t));
}

/* ============================================================================
 * Context Management
 * ============================================================================ */

int mimi_core_set_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, const char *value)
{
    if (ctx == NULL || session_id == NULL || key == NULL || value == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    mimi_core_session_t *session = find_session(ctx, session_id);
    if (session == NULL) {
        return MIMI_CORE_ERR_SESSION_NOT_FOUND;
    }
    
    return set_context_entry(session, key, value);
}

int mimi_core_get_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, char **value)
{
    if (ctx == NULL || session_id == NULL || key == NULL || value == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    mimi_core_session_t *session = find_session(ctx, session_id);
    if (session == NULL) {
        return MIMI_CORE_ERR_SESSION_NOT_FOUND;
    }
    
    const char *entry_value = get_context_entry(session, key);
    if (entry_value == NULL) {
        return MIMI_CORE_ERR_NOT_FOUND;
    }
    
    *value = strdup(entry_value);
    if (*value == NULL) {
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    return MIMI_CORE_OK;
}

int mimi_core_delete_context(mimi_core_ctx_t *ctx, const char *session_id,
                             const char *key)
{
    if (ctx == NULL || session_id == NULL || key == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    mimi_core_session_t *session = find_session(ctx, session_id);
    if (session == NULL) {
        return MIMI_CORE_ERR_SESSION_NOT_FOUND;
    }
    
    mimi_core_context_entry_t **prev = &session->context;
    mimi_core_context_entry_t *entry = session->context;
    
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            *prev = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            return MIMI_CORE_OK;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    return MIMI_CORE_ERR_NOT_FOUND;
}

int mimi_core_clear_context(mimi_core_ctx_t *ctx, const char *session_id)
{
    if (ctx == NULL || session_id == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    mimi_core_session_t *session = find_session(ctx, session_id);
    if (session == NULL) {
        return MIMI_CORE_ERR_SESSION_NOT_FOUND;
    }
    
    /* Free all context entries */
    mimi_core_context_entry_t *entry = session->context;
    while (entry != NULL) {
        mimi_core_context_entry_t *next = entry->next;
        free(entry->key);
        free(entry->value);
        free(entry);
        entry = next;
    }
    
    session->context = NULL;
    return MIMI_CORE_OK;
}

/* ============================================================================
 * Session Management
 * ============================================================================ */

int mimi_core_session_create(mimi_core_ctx_t *ctx, const char *session_id)
{
    if (ctx == NULL || session_id == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    if (find_session(ctx, session_id) != NULL) {
        return MIMI_CORE_ERR_SESSION_EXISTS;
    }
    
    mimi_core_session_t *session = create_session(session_id);
    if (session == NULL) {
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    session->next = ctx->sessions;
    ctx->sessions = session;
    return MIMI_CORE_OK;
}

int mimi_core_session_delete(mimi_core_ctx_t *ctx, const char *session_id)
{
    if (ctx == NULL || session_id == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    mimi_core_session_t **prev = &ctx->sessions;
    mimi_core_session_t *session = ctx->sessions;
    
    while (session != NULL) {
        if (strcmp(session->session_id, session_id) == 0) {
            *prev = session->next;
            free_session(session);
            return MIMI_CORE_OK;
        }
        prev = &session->next;
        session = session->next;
    }
    
    return MIMI_CORE_ERR_SESSION_NOT_FOUND;
}

int mimi_core_session_list(mimi_core_ctx_t *ctx, char ***sessions, int *count)
{
    if (ctx == NULL || sessions == NULL || count == NULL) {
        return MIMI_CORE_ERR_INVALID_ARG;
    }
    
    /* Count sessions */
    int session_count = 0;
    mimi_core_session_t *session = ctx->sessions;
    while (session != NULL) {
        session_count++;
        session = session->next;
    }
    
    if (session_count == 0) {
        *sessions = NULL;
        *count = 0;
        return MIMI_CORE_OK;
    }
    
    /* Allocate array */
    *sessions = (char**)malloc(session_count * sizeof(char*));
    if (*sessions == NULL) {
        return MIMI_CORE_ERR_NO_MEMORY;
    }
    
    /* Fill array */
    session = ctx->sessions;
    for (int i = 0; i < session_count; i++) {
        (*sessions)[i] = strdup(session->session_id);
        if ((*sessions)[i] == NULL) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) {
                free((*sessions)[j]);
            }
            free(*sessions);
            *sessions = NULL;
            *count = 0;
            return MIMI_CORE_ERR_NO_MEMORY;
        }
        session = session->next;
    }
    
    *count = session_count;
    return MIMI_CORE_OK;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* mimi_core_version(void)
{
    return MIMI_CORE_VERSION;
}

const char* mimi_core_strerror(int error)
{
    switch (error) {
        case MIMI_CORE_OK:
            return "Success";
        case MIMI_CORE_ERR_INVALID_ARG:
            return "Invalid argument";
        case MIMI_CORE_ERR_NO_MEMORY:
            return "Memory allocation failed";
        case MIMI_CORE_ERR_INIT_FAILED:
            return "Initialization failed";
        case MIMI_CORE_ERR_NOT_FOUND:
            return "Resource not found";
        case MIMI_CORE_ERR_API_ERROR:
            return "External API error";
        case MIMI_CORE_ERR_TIMEOUT:
            return "Operation timeout";
        case MIMI_CORE_ERR_SESSION_EXISTS:
            return "Session already exists";
        case MIMI_CORE_ERR_SESSION_NOT_FOUND:
            return "Session not found";
        default:
            return "Unknown error";
    }
}
