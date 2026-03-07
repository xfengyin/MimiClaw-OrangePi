/*
 * Memory Cache Header - LRU Cache for Performance
 */

#ifndef CACHE_H
#define CACHE_H

#include <time.h>
#include <stdbool.h>

#define CACHE_MAX_ENTRIES 1000
#define CACHE_DEFAULT_TTL 300  // 5 minutes
#define CACHE_MAX_KEY_LENGTH 256
#define CACHE_MAX_VALUE_LENGTH 4096

typedef struct cache_entry {
    char key[CACHE_MAX_KEY_LENGTH];
    char value[CACHE_MAX_VALUE_LENGTH];
    time_t created_at;
    time_t expires_at;
    struct cache_entry *prev;
    struct cache_entry *next;
} cache_entry_t;

typedef struct {
    cache_entry_t *head;  // Most recently used
    cache_entry_t *tail;  // Least recently used
    int size;
    int max_size;
    int default_ttl;
    pthread_mutex_t mutex;
} cache_t;

// Function prototypes
int cache_init(cache_t *cache, int max_size, int default_ttl);
void cache_close(cache_t *cache);
int cache_set(cache_t *cache, const char *key, const char *value, int ttl);
const char* cache_get(cache_t *cache, const char *key);
void cache_delete(cache_t *cache, const char *key);
void cache_clear(cache_t *cache);
int cache_size(cache_t *cache);

#endif // CACHE_H