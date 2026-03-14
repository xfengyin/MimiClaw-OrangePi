/**
 * @file gateway.c
 * @brief MimiClaw Gateway - Core Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "gateway.h"
#include "mimi_core.h"
#include "mimi_memory.h"
#include "mimi_config.h"
#include "mimi_tools.h"

#define MAX_SESSIONS 1000

/* Global state */
static gateway_config_t g_config;
static mimi_core_ctx_t *g_core = NULL;
static mimi_mem_pool_t *g_memory = NULL;
static mimi_tool_registry_t *g_tools = NULL;
static gateway_session_t **g_sessions = NULL;
static int g_session_count = 0;
static pthread_mutex_t g_session_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Session management */
static char* generate_session_id(void) {
    static char id[64];
    snprintf(id, sizeof(id), "session_%ld_%d", 
             (long)time(NULL), rand() % 10000);
    return id;
}

int gateway_init(const gateway_config_t *config) {
    if (!config) return -1;

    memcpy(&g_config, config, sizeof(gateway_config_t));

    /* Initialize memory pool */
    mimi_mem_config_t mem_cfg = {
        .db_path = "./mimi.db",
        .pool_size = 4,
        .max_idle_time = 60
    };
    g_memory = mimi_mem_pool_create(&mem_cfg);
    if (!g_memory) {
        fprintf(stderr, "Failed to create memory pool\n");
        return -1;
    }

    /* Initialize core */
    mimi_core_config_t core_cfg = {
        .api_key = g_config.api_key,
        .model = g_config.model,
        .max_tokens = 2048,
        .temperature = 0.7f
    };
    if (mimi_core_init(&g_core, &core_cfg) != 0) {
        fprintf(stderr, "Failed to initialize core\n");
        return -1;
    }

    /* Initialize tool registry */
    g_tools = mimi_registry_create();
    if (!g_tools) {
        fprintf(stderr, "Failed to create tool registry\n");
        return -1;
    }

    /* Allocate session array */
    g_sessions = calloc(MAX_SESSIONS, sizeof(gateway_session_t*));

    printf("[+] Memory pool initialized\n");
    printf("[+] Core initialized (model: %s)\n", g_config.model);
    printf("[+] Tool registry initialized\n");

    return 0;
}

int gateway_run(void) {
    /* CLI mode - interactive chat */
    char input[4096] = {0};
    char *response = NULL;
    char *current_session = generate_session_id();

    printf("Session: %s\n", current_session);
    printf("Type 'quit' to exit, 'new' for new session\n\n");
    printf("> ");

    while (g_running && fgets(input, sizeof(input), stdin)) {
        /* Remove newline */
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        if (strcmp(input, "new") == 0) {
            current_session = generate_session_id();
            printf("New session: %s\n> ", current_session);
            continue;
        }

        if (strlen(input) == 0) {
            printf("> ");
            continue;
        }

        /* Handle chat */
        if (gateway_handle_chat(current_session, input, &response) == 0) {
            if (response) {
                printf("%s\n\n", response);
                free(response);
            }
        } else {
            printf("Error: Failed to process message\n\n");
        }

        printf("> ");
    }

    return 0;
}

void gateway_stop(void) {
    printf("\n[*] Shutting down gateway...\n");
}

void gateway_cleanup(void) {
    pthread_mutex_lock(&g_session_mutex);

    /* Free sessions */
    for (int i = 0; i < g_session_count; i++) {
        free(g_sessions[i]);
    }
    free(g_sessions);

    pthread_mutex_unlock(&g_session_mutex);

    /* Cleanup components */
    if (g_tools) mimi_registry_destroy(g_tools);
    if (g_core) mimi_core_destroy(g_core);
    if (g_memory) mimi_mem_pool_destroy(g_memory);

    printf("[+] Gateway cleanup complete\n");
}

/* CLI command handlers */
int gateway_handle_chat(const char *session_id, const char *message, char **response) {
    if (!session_id || !message || !response) return -1;

    return mimi_core_chat(g_core, session_id, message, response);
}

int gateway_handle_memory_read(const char *key, char **value) {
    if (!key || !value) return -1;
    return mimi_mem_read(g_memory, key, value);
}

int gateway_handle_memory_write(const char *key, const char *value) {
    if (!key || !value) return -1;
    return mimi_mem_write(g_memory, key, value);
}

int gateway_handle_session_list(char ***sessions, int *count) {
    if (!sessions || !count) return -1;
    return mimi_mem_session_list(g_memory, sessions, count);
}

int gateway_handle_session_clear(const char *session_id) {
    if (!session_id) return -1;
    return mimi_mem_session_delete(g_memory, session_id);
}