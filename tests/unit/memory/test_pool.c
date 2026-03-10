/**
 * @file test_pool.c
 * @brief Unit tests for Memory Library - Connection Pool
 * @coverage mimi_memory connection pooling, concurrent access, WAL mode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "mimi_memory.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("✓\n"); \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { \
    printf("✗ FAILED: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
} } while(0)

static const char *TEST_DB_PATH = "/tmp/test_mimi_pool.db";

/* Thread test data */
typedef struct {
    mimi_mem_pool_t *pool;
    int thread_id;
    int success_count;
} thread_test_data_t;

/* Thread function for concurrent access test */
static void *thread_write_read(void *arg)
{
    thread_test_data_t *data = (thread_test_data_t *)arg;
    
    for (int i = 0; i < 10; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "thread_%d_key_%d", data->thread_id, i);
        snprintf(value, sizeof(value), "thread_%d_value_%d", data->thread_id, i);
        
        if (mimi_mem_write(data->pool, key, value) == MIMI_MEMORY_OK) {
            char *read_value = NULL;
            if (mimi_mem_read(data->pool, key, &read_value) == MIMI_MEMORY_OK) {
                if (strcmp(read_value, value) == 0) {
                    data->success_count++;
                }
                free(read_value);
            }
        }
    }
    
    return NULL;
}

/* Test pool with different sizes */
TEST(test_pool_different_sizes)
{
    int pool_sizes[] = {1, 2, 4, 8, 16};
    
    for (size_t i = 0; i < sizeof(pool_sizes)/sizeof(pool_sizes[0]); i++) {
        mimi_mem_config_t config = {
            .db_path = TEST_DB_PATH,
            .pool_size = pool_sizes[i],
            .max_idle_time = 300,
            .enable_wal = 1
        };
        
        mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
        ASSERT(pool != NULL);
        
        /* Basic operation test */
        char key[64], value[64];
        snprintf(key, sizeof(key), "pool_size_%d", pool_sizes[i]);
        snprintf(value, sizeof(value), "value_for_pool_%d", pool_sizes[i]);
        
        ASSERT(mimi_mem_write(pool, key, value) == MIMI_MEMORY_OK);
        
        char *read_value = NULL;
        ASSERT(mimi_mem_read(pool, key, &read_value) == MIMI_MEMORY_OK);
        ASSERT(strcmp(read_value, value) == 0);
        free(read_value);
        
        mimi_mem_pool_destroy(pool);
    }
}

/* Test WAL mode enabled/disabled */
TEST(test_pool_wal_mode)
{
    /* WAL enabled */
    mimi_mem_config_t config_wal = {
        .db_path = TEST_DB_PATH,
        .pool_size = 4,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool_wal = mimi_mem_pool_create(&config_wal);
    ASSERT(pool_wal != NULL);
    
    ASSERT(mimi_mem_write(pool_wal, "wal_test", "wal_value") == MIMI_MEMORY_OK);
    char *value = NULL;
    ASSERT(mimi_mem_read(pool_wal, "wal_test", &value) == MIMI_MEMORY_OK);
    ASSERT(strcmp(value, "wal_value") == 0);
    free(value);
    
    mimi_mem_pool_destroy(pool_wal);
    
    /* WAL disabled */
    mimi_mem_config_t config_no_wal = {
        .db_path = TEST_DB_PATH,
        .pool_size = 4,
        .max_idle_time = 300,
        .enable_wal = 0
    };
    
    mimi_mem_pool_t *pool_no_wal = mimi_mem_pool_create(&config_no_wal);
    ASSERT(pool_no_wal != NULL);
    
    ASSERT(mimi_mem_write(pool_no_wal, "no_wal_test", "no_wal_value") == MIMI_MEMORY_OK);
    value = NULL;
    ASSERT(mimi_mem_read(pool_no_wal, "no_wal_test", &value) == MIMI_MEMORY_OK);
    ASSERT(strcmp(value, "no_wal_value") == 0);
    free(value);
    
    mimi_mem_pool_destroy(pool_no_wal);
}

/* Test concurrent access from multiple threads */
TEST(test_pool_concurrent_access)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 8,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    const int NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    thread_test_data_t thread_data[NUM_THREADS];
    
    /* Initialize thread data */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].pool = pool;
        thread_data[i].thread_id = i;
        thread_data[i].success_count = 0;
    }
    
    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_write_read, &thread_data[i]);
    }
    
    /* Wait for threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Verify all threads succeeded */
    int total_success = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("[%d successes] ", thread_data[i].success_count);
        total_success += thread_data[i].success_count;
    }
    
    /* All operations should succeed */
    ASSERT(total_success == NUM_THREADS * 10);
    
    mimi_mem_pool_destroy(pool);
}

/* Test pool vacuum operation */
TEST(test_pool_vacuum)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write and delete to create fragmentation */
    for (int i = 0; i < 100; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "vacuum_key_%d", i);
        snprintf(value, sizeof(value), "vacuum_value_%d", i);
        mimi_mem_write(pool, key, value);
        mimi_mem_delete(pool, key);
    }
    
    /* Run vacuum */
    int ret = mimi_mem_vacuum(pool);
    ASSERT(ret == MIMI_MEMORY_OK);
    
    mimi_mem_pool_destroy(pool);
}

/* Test pool idle timeout */
TEST(test_pool_idle_timeout)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 4,
        .max_idle_time = 1,  /* 1 second timeout */
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Basic operation */
    ASSERT(mimi_mem_write(pool, "timeout_test", "value") == MIMI_MEMORY_OK);
    
    /* Wait for timeout */
    sleep(2);
    
    /* Should still work after timeout (pool should reconnect) */
    char *value = NULL;
    int ret = mimi_mem_read(pool, "timeout_test", &value);
    ASSERT(ret == MIMI_MEMORY_OK);
    ASSERT(strcmp(value, "value") == 0);
    free(value);
    
    mimi_mem_pool_destroy(pool);
}

/* Test last error retrieval */
TEST(test_pool_last_error)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Perform operation */
    mimi_mem_write(pool, "error_test", "value");
    
    /* Get last error (should be empty/success) */
    const char *last_error = mimi_mem_last_error(pool);
    /* May be NULL or empty on success */
    
    mimi_mem_pool_destroy(pool);
}

/* Test pool with very large values */
TEST(test_pool_large_values)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Create large value (10KB) */
    char *large_value = malloc(10240);
    ASSERT(large_value != NULL);
    memset(large_value, 'X', 10239);
    large_value[10239] = '\0';
    
    ASSERT(mimi_mem_write(pool, "large_key", large_value) == MIMI_MEMORY_OK);
    
    char *read_value = NULL;
    ASSERT(mimi_mem_read(pool, "large_key", &read_value) == MIMI_MEMORY_OK);
    ASSERT(strcmp(read_value, large_value) == 0);
    
    free(read_value);
    free(large_value);
    mimi_mem_pool_destroy(pool);
}

int main(void)
{
    printf("=== libmimi-memory Pool Tests ===\n\n");
    
    RUN_TEST(test_pool_different_sizes);
    RUN_TEST(test_pool_wal_mode);
    RUN_TEST(test_pool_concurrent_access);
    RUN_TEST(test_pool_vacuum);
    RUN_TEST(test_pool_idle_timeout);
    RUN_TEST(test_pool_last_error);
    RUN_TEST(test_pool_large_values);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    /* Cleanup test database */
    remove(TEST_DB_PATH);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
