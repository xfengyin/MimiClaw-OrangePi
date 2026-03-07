/*
 * Memory Cache Implementation - LRU Cache
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cache.h"
#include "logger.h"

static void cache_move_to_front(cache_t *cache, cache_entry_t *entry) {
    if (!cache || !entry || entry == cache->head) return;
    
    // Remove from current position
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == cache->tail) cache->tail = entry->prev;
    
    // Move to front
    entry->next = cache->head;
    entry->prev = NULL;
    if (cache->head) cache->head->prev = entry;
    cache->head = entry;
}

static void cache_remove_entry(cache_t *cache, cache_entry_t *entry) {
    if (!cache || !entry) return;
    
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == cache->head) cache->head = entry->next;
    if (entry == cache->tail) cache->tail = entry->prev;
    
    free(entry);
    cache->size--;
}

int cache_init(cache_t *cache, int max_size, int default_ttl) {
    if (!cache || max_size <= 0) return -1;
    
    memset(cache, 0, sizeof(cache_t));
    cache->max_size = max_size;
    cache->default_ttl = default_ttl > 0 ? default_ttl : CACHE_DEFAULT_TTL;
    pthread_mutex_init(&cache->mutex, NULL);
    
    LOG_INFO("Cache initialized: max_size=%d, default_ttl=%d", max_size, cache->default_ttl);
    return 0;
}

void cache_close(cache_t *cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *current = cache->head;
    while (current) {
        cache_entry_t *next = current->next;
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    pthread_mutex_destroy(&cache->mutex);
    
    LOG_INFO("Cache closed");
}

int cache_set(cache_t *cache, const char *key, const char *value, int ttl) {
    if (!cache || !key || !value) return -1;
    
    if (strlen(key) >= CACHE_MAX_KEY_LENGTH || strlen(value) >= CACHE_MAX_VALUE_LENGTH) {
        LOG_WARN("Cache key or value too long");
        return -1;
    }
    
    pthread_mutex_lock(&cache->mutex);
    
    // Check if key already exists
    cache_entry_t *current = cache->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Update existing entry
            strncpy(current->value, value, CACHE_MAX_VALUE_LENGTH - 1);
            current->created_at = time(NULL);
            current->expires_at = current->created_at + (ttl > 0 ? ttl : cache->default_ttl);
            cache_move_to_front(cache, current);
            pthread_mutex_unlock(&cache->mutex);
            LOG_DEBUG("Cache updated: %s", key);
            return 0;
        }
        current = current->next;
    }
    
    // Evict oldest if at capacity
    if (cache->size >= cache->max_size && cache->tail) {
        LOG_DEBUG("Cache full, evicting oldest entry: %s", cache->tail->key);
        cache_remove_entry(cache, cache->tail);
    }
    
    // Create new entry
    cache_entry_t *new_entry = malloc(sizeof(cache_entry_t));
    if (!new_entry) {
        pthread_mutex_unlock(&cache->mutex);
        return -1;
    }
    
    strncpy(new_entry->key, key, CACHE_MAX_KEY_LENGTH - 1);
    strncpy(new_entry->value, value, CACHE_MAX_VALUE_LENGTH - 1);
    new_entry->created_at = time(NULL);
    new_entry->expires_at = new_entry->created_at + (ttl > 0 ? ttl : cache->default_ttl);
    new_entry->prev = NULL;
    new_entry->next = cache->head;
    
    if (cache->head) cache->head->prev = new_entry;
    cache->head = new_entry;
    if (!cache->tail) cache->tail = new_entry;
    
    cache->size++;
    pthread_mutex_unlock(&cache->mutex);
    
    LOG_DEBUG("Cache set: %s", key);
    return 0;
}

const char* cache_get(cache_t *cache, const char *key) {
    if (!cache || !key) return NULL;
    
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *current = cache->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Check expiration
            if (time(NULL) > current->expires_at) {
                LOG_DEBUG("Cache entry expired: %s", key);
                cache_remove_entry(cache, current);
                pthread_mutex_unlock(&cache->mutex);
                return NULL;
            }
            
            // Move to front (LRU)
            cache_move_to_front(cache, current);
            pthread_mutex_unlock(&cache->mutex);
            LOG_DEBUG("Cache hit: %s", key);
            return current->value;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    LOG_DEBUG("Cache miss: %s", key);
    return NULL;
}

void cache_delete(cache_t *cache, const char *key) {
    if (!cache || !key) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *current = cache->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            cache_remove_entry(cache, current);
            LOG_DEBUG("Cache deleted: %s", key);
            break;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&cache->mutex);
}

void cache_clear(cache_t *cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *current = cache->head;
    while (current) {
        cache_entry_t *next = current->next;
        free(current);
        current = next;
    }
    
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    
    pthread_mutex_unlock(&cache->mutex);
    
    LOG_INFO("Cache cleared");
}

int cache_size(cache_t *cache) {
    if (!cache) return 0;
    return cache->size;
}