/*
 * WebSocket Server Header
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <stdbool.h>
#include <pthread.h>
#include "../core/config.h"
#include "../memory/memory_store.h"

#define WS_MAX_CLIENTS 10
#define WS_BUFFER_SIZE 4096
#define WS_PORT 18789

typedef struct {
    int socket;
    bool connected;
    char session_id[64];
    pthread_t thread;
} ws_client_t;

typedef struct {
    int server_socket;
    int port;
    bool running;
    ws_client_t clients[WS_MAX_CLIENTS];
    pthread_t accept_thread;
    app_config_t *config;
    memory_store_t *memory;
} ws_server_t;

// Function prototypes
int ws_server_init(ws_server_t *server, int port, app_config_t *config, memory_store_t *memory);
void ws_server_close(ws_server_t *server);
int ws_server_start(ws_server_t *server);
void ws_server_stop(ws_server_t *server);
int ws_server_broadcast(ws_server_t *server, const char *message);

#endif // WEBSOCKET_SERVER_H