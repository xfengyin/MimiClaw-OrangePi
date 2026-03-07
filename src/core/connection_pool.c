/*
 * Connection Pool Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "connection_pool.h"
#include "logger.h"

int connection_pool_init(connection_pool_t *pool, int size) {
    if (!pool || size <= 0 || size > MAX_POOL_SIZE) return -1;
    
    memset(pool, 0, sizeof(connection_pool_t));
    pool->size = size;
    pthread_mutex_init(&pool->mutex, NULL);
    
    for (int i = 0; i < size; i++) {
        pool->connections[i].curl = curl_easy_init();
        if (!pool->connections[i].curl) {
            LOG_ERROR("Failed to initialize CURL handle %d", i);
            // Cleanup already initialized handles
            for (int j = 0; j < i; j++) {
                curl_easy_cleanup(pool->connections[j].curl);
            }
            pthread_mutex_destroy(&pool->mutex);
            return -1;
        }
        pool->connections[i].in_use = false;
        pool->connections[i].last_used = time(NULL);
        
        // Set common options
        curl_easy_setopt(pool->connections[i].curl, CURLOPT_TIMEOUT, CONNECTION_TIMEOUT);
        curl_easy_setopt(pool->connections[i].curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(pool->connections[i].curl, CURLOPT_MAXREDIRS, 5L);
    }
    
    LOG_INFO("Connection pool initialized with %d connections", size);
    return 0;
}

void connection_pool_close(connection_pool_t *pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    for (int i = 0; i < pool->size; i++) {
        if (pool->connections[i].curl) {
            curl_easy_cleanup(pool->connections[i].curl);
            pool->connections[i].curl = NULL;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    
    LOG_INFO("Connection pool closed");
}

CURL* connection_pool_acquire(connection_pool_t *pool) {
    if (!pool) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    // Find available connection
    for (int i = 0; i < pool->size; i++) {
        if (!pool->connections[i].in_use) {
            pool->connections[i].in_use = true;
            pool->connections[i].last_used = time(NULL);
            
            // Reset handle for reuse
            curl_easy_reset(pool->connections[i].curl);
            curl_easy_setopt(pool->connections[i].curl, CURLOPT_TIMEOUT, CONNECTION_TIMEOUT);
            curl_easy_setopt(pool->connections[i].curl, CURLOPT_FOLLOWLOCATION, 1L);
            
            // Set proxy if configured
            if (pool->proxy_enabled && strlen(pool->proxy_url) > 0) {
                curl_easy_setopt(pool->connections[i].curl, CURLOPT_PROXY, pool->proxy_url);
            }
            
            pthread_mutex_unlock(&pool->mutex);
            LOG_DEBUG("Connection %d acquired from pool", i);
            return pool->connections[i].curl;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    LOG_WARN("No available connections in pool");
    return NULL;
}

void connection_pool_release(connection_pool_t *pool, CURL *curl) {
    if (!pool || !curl) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    for (int i = 0; i < pool->size; i++) {
        if (pool->connections[i].curl == curl) {
            pool->connections[i].in_use = false;
            pool->connections[i].last_used = time(NULL);
            LOG_DEBUG("Connection %d released to pool", i);
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
}

void connection_pool_set_proxy(connection_pool_t *pool, const char *proxy_url) {
    if (!pool || !proxy_url) return;
    
    pthread_mutex_lock(&pool->mutex);
    strncpy(pool->proxy_url, proxy_url, sizeof(pool->proxy_url) - 1);
    pool->proxy_enabled = true;
    pthread_mutex_unlock(&pool->mutex);
    
    LOG_INFO("Proxy configured for connection pool: %s", proxy_url);
}