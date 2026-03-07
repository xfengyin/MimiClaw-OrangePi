/*
 * HTTP Server Header - Health Check and Metrics Endpoints
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>
#include "../core/config.h"
#include "../core/metrics.h"

#define HTTP_MAX_PATH 256
#define HTTP_MAX_BODY 4096

typedef struct {
    int port;
    bool running;
    int server_socket;
    pthread_t thread;
    app_config_t *config;
    metrics_registry_t *metrics;
} http_server_t;

// Function prototypes
int http_server_init(http_server_t *server, int port, app_config_t *config, metrics_registry_t *metrics);
void http_server_close(http_server_t *server);
int http_server_start(http_server_t *server);
void http_server_stop(http_server_t *server);

#endif // HTTP_SERVER_H