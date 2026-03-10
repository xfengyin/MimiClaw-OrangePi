/**
 * @file example_memory.c
 * @brief Example: Using libmimi-memory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mimi_memory.h"

int main(void)
{
    printf("=== MimiMemory Example ===\n\n");
    printf("Library version: %s\n\n", mimi_mem_version());
    
    /* Create pool */
    mimi_mem_config_t config = {
        .db_path = ":memory:",  /* In-memory database for testing */
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    if (pool == NULL) {
        fprintf(stderr, "Pool create failed: %s\n", mimi_mem_last_error(pool));
        return 1;
    }
    printf("✓ Memory pool created\n");
    
    /* Create session */
    int ret = mimi_mem_session_create(pool, "session-1");
    if (ret != MIMI_MEMORY_OK) {
        fprintf(stderr, "Session create failed: %s\n", mimi_mem_strerror(ret));
        mimi_mem_pool_destroy(pool);
        return 1;
    }
    printf("✓ Session created\n");
    
    /* Append messages */
    int64_t msg_id;
    ret = mimi_mem_message_append(pool, "session-1", "user", "Hello!", &msg_id);
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Message appended (ID: %ld)\n", (long)msg_id);
    }
    
    ret = mimi_mem_message_append(pool, "session-1", "assistant", "Hi there!", NULL);
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Response appended\n");
    }
    
    /* Query messages */
    mimi_mem_message_t **messages;
    int count;
    ret = mimi_mem_message_query(pool, "session-1", 10, &messages, &count);
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Retrieved %d messages:\n", count);
        for (int i = 0; i < count; i++) {
            printf("  [%s] %s\n", messages[i]->role, messages[i]->content);
            mimi_mem_message_free(messages[i]);
        }
        free(messages);
    }
    
    /* Write memory */
    ret = mimi_mem_write(pool, "user_preference", "dark_mode");
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Memory written\n");
    }
    
    /* Read memory */
    char *value;
    ret = mimi_mem_read(pool, "user_preference", &value);
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Memory read: %s\n", value);
        free(value);
    }
    
    /* Search memory */
    mimi_mem_entry_t **results;
    ret = mimi_mem_search(pool, "preference", &results, &count);
    if (ret == MIMI_MEMORY_OK && count > 0) {
        printf("✓ Search found %d results:\n", count);
        for (int i = 0; i < count; i++) {
            printf("  %s = %s\n", results[i]->key, results[i]->value);
            mimi_mem_entry_free(results[i]);
        }
        free(results);
    }
    
    /* List sessions */
    char **sessions;
    ret = mimi_mem_session_list(pool, &sessions, &count);
    if (ret == MIMI_MEMORY_OK) {
        printf("✓ Sessions (%d):\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - %s\n", sessions[i]);
            free(sessions[i]);
        }
        free(sessions);
    }
    
    /* Cleanup */
    mimi_mem_pool_destroy(pool);
    printf("✓ Memory pool destroyed\n");
    
    printf("\n=== Example Complete ===\n");
    return 0;
}
