/**
 * @file mimi-gateway.h
 * @brief MimiClaw Gateway - Multi-protocol AI Assistant Gateway
 */

#ifndef MIMI_GATEWAY_H
#define MIMI_GATEWAY_H

#include <stddef.h>

/* Gateway configuration */
typedef struct {
    const char *bind_addr;
    int port;
    const char *api_key;
    const char *model;
    int max_sessions;
} gateway_config_t;

/* Session context */
typedef struct {
    char session_id[64];
    void *core_ctx;
    time_t created_at;
} gateway_session_t;

/* Initialize gateway with config */
int gateway_init(const gateway_config_t *config);

/* Run gateway (blocking) */
int gateway_run(void);

/* Stop gateway */
void gateway_stop(void);

/* Cleanup gateway */
void gateway_cleanup(void);

/* CLI command handlers */
int gateway_handle_chat(const char *session_id, const char *message, char **response);
int gateway_handle_memory_read(const char *key, char **value);
int gateway_handle_memory_write(const char *key, const char *value);
int gateway_handle_session_list(char ***sessions, int *count);
int gateway_handle_session_clear(const char *session_id);

#endif /* MIMI_GATEWAY_H */