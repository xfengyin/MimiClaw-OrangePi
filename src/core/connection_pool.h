/*
 * Connection Pool Header - HTTP Connection Pooling for Performance
 */

#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_POOL_SIZE 10
#define CONNECTION_TIMEOUT 30
#define MAX_IDLE_TIME 300  // 5 minutes

typedef struct {
    CURL *curl;
    bool in_use;
    time_t last_used;
} pooled_connection_t;

typedef struct {
    pooled_connection_t connections[MAX_POOL_SIZE];
    int size;
    pthread_mutex_t mutex;
    char proxy_url[256];
    bool proxy_enabled;
} connection_pool_t;

// Function prototypes
int connection_pool_init(connection_pool_t *pool, int size);
void connection_pool_close(connection_pool_t *pool);
CURL* connection_pool_acquire(connection_pool_t *pool);
void connection_pool_release(connection_pool_t *pool, CURL *curl);
void connection_pool_set_proxy(connection_pool_t *pool, const char *proxy_url);

#endif // CONNECTION_POOL_H