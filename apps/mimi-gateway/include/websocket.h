/**
 * @file websocket.h
 * @brief WebSocket Server Support
 */

#ifndef MIMI_WEBSOCKET_H
#define MIMI_WEBSOCKET_H

#include <stdint.h>
#include <pthread.h>

typedef struct websocket_server websocket_server_t;

/**
 * Initialize WebSocket server
 * @param port Listen port
 * @param handler Callback: void handler(session_id, message, response_ptr)
 * @return Server instance or NULL on error
 */
websocket_server_t* websocket_init(int port,
    void (*handler)(const char *session_id, const char *message, char **response));

/**
 * Start accepting connections
 */
int websocket_start(websocket_server_t *server);

/**
 * Stop server and disconnect all clients
 */
void websocket_stop(websocket_server_t *server);

/**
 * Cleanup and free server
 */
void websocket_cleanup(websocket_server_t *server);

#endif /* MIMI_WEBSOCKET_H */