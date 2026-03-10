/**
 * @file test_memory.c
 * @brief Unit tests for Memory Library - Basic Operations
 * @coverage mimi_memory pool creation, basic CRUD operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

static const char *TEST_DB_PATH = "/tmp/test_mimi_memory.db";

/* Test pool creation and destruction */
TEST(test_memory_pool_create_destroy)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 4,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    mimi_mem_pool_destroy(pool);
}

/* Test pool creation with invalid config */
TEST(test_memory_pool_invalid_config)
{
    /* NULL config */
    mimi_mem_pool_t *pool = mimi_mem_pool_create(NULL);
    ASSERT(pool == NULL);
    
    /* Empty path */
    mimi_mem_config_t config = {0};
    pool = mimi_mem_pool_create(&config);
    ASSERT(pool == NULL);
}

/* Test basic write and read */
TEST(test_memory_write_read)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write */
    int ret = mimi_mem_write(pool, "test_key", "test_value");
    ASSERT(ret == MIMI_MEMORY_OK);
    
    /* Read */
    char *value = NULL;
    ret = mimi_mem_read(pool, "test_key", &value);
    ASSERT(ret == MIMI_MEMORY_OK);
    ASSERT(value != NULL);
    ASSERT(strcmp(value, "test_value") == 0);
    
    free(value);
    mimi_mem_pool_destroy(pool);
}

/* Test read non-existent key */
TEST(test_memory_read_nonexistent)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    char *value = NULL;
    int ret = mimi_mem_read(pool, "nonexistent_key", &value);
    ASSERT(ret == MIMI_MEMORY_ERR_NOT_FOUND);
    ASSERT(value == NULL);
    
    mimi_mem_pool_destroy(pool);
}

/* Test delete operation */
TEST(test_memory_delete)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write then delete */
    ASSERT(mimi_mem_write(pool, "delete_key", "delete_value") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_delete(pool, "delete_key") == MIMI_MEMORY_OK);
    
    /* Verify deleted */
    char *value = NULL;
    int ret = mimi_mem_read(pool, "delete_key", &value);
    ASSERT(ret == MIMI_MEMORY_ERR_NOT_FOUND);
    
    /* Delete non-existent */
    ret = mimi_mem_delete(pool, "nonexistent");
    ASSERT(ret == MIMI_MEMORY_ERR_NOT_FOUND);
    
    mimi_mem_pool_destroy(pool);
}

/* Test update existing key */
TEST(test_memory_update)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write initial value */
    ASSERT(mimi_mem_write(pool, "update_key", "value1") == MIMI_MEMORY_OK);
    
    /* Update */
    ASSERT(mimi_mem_write(pool, "update_key", "value2") == MIMI_MEMORY_OK);
    
    /* Verify updated */
    char *value = NULL;
    ASSERT(mimi_mem_read(pool, "update_key", &value) == MIMI_MEMORY_OK);
    ASSERT(strcmp(value, "value2") == 0);
    free(value);
    
    mimi_mem_pool_destroy(pool);
}

/* Test multiple key-value pairs */
TEST(test_memory_multiple_entries)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write multiple entries */
    for (int i = 0; i < 20; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        ASSERT(mimi_mem_write(pool, key, value) == MIMI_MEMORY_OK);
    }
    
    /* Read all back */
    for (int i = 0; i < 20; i++) {
        char key[64], expected[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(expected, sizeof(expected), "value_%d", i);
        
        char *value = NULL;
        ASSERT(mimi_mem_read(pool, key, &value) == MIMI_MEMORY_OK);
        ASSERT(strcmp(value, expected) == 0);
        free(value);
    }
    
    mimi_mem_pool_destroy(pool);
}

/* Test special characters in values */
TEST(test_memory_special_characters)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    const char *special_values[] = {
        "value with spaces",
        "value\twith\ttabs",
        "value\nwith\nnewlines",
        "value'with'quotes",
        "value\"with\"doublequotes",
        "SELECT * FROM users; -- SQL injection attempt",
        "Unicode: 你好世界 🌍 émojis",
        ""  /* Empty value */
    };
    
    for (size_t i = 0; i < sizeof(special_values)/sizeof(special_values[0]); i++) {
        char key[64];
        snprintf(key, sizeof(key), "special_%zu", i);
        
        ASSERT(mimi_mem_write(pool, key, special_values[i]) == MIMI_MEMORY_OK);
        
        char *value = NULL;
        ASSERT(mimi_mem_read(pool, key, &value) == MIMI_MEMORY_OK);
        ASSERT(strcmp(value, special_values[i]) == 0);
        free(value);
    }
    
    mimi_mem_pool_destroy(pool);
}

/* Test version and error functions */
TEST(test_memory_utility_functions)
{
    ASSERT(mimi_mem_version() != NULL);
    ASSERT(mimi_mem_strerror(MIMI_MEMORY_OK) != NULL);
    ASSERT(mimi_mem_strerror(MIMI_MEMORY_ERR_INVALID_ARG) != NULL);
    ASSERT(mimi_mem_strerror(-999) != NULL); /* Unknown error */
}

int main(void)
{
    printf("=== libmimi-memory Basic Tests ===\n\n");
    
    RUN_TEST(test_memory_pool_create_destroy);
    RUN_TEST(test_memory_pool_invalid_config);
    RUN_TEST(test_memory_write_read);
    RUN_TEST(test_memory_read_nonexistent);
    RUN_TEST(test_memory_delete);
    RUN_TEST(test_memory_update);
    RUN_TEST(test_memory_multiple_entries);
    RUN_TEST(test_memory_special_characters);
    RUN_TEST(test_memory_utility_functions);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    /* Cleanup test database */
    remove(TEST_DB_PATH);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
