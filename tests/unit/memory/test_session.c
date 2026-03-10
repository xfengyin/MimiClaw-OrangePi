/**
 * @file test_session.c
 * @brief Unit tests for Memory Library - Session Management
 * @coverage mimi_memory session CRUD, message storage, session listing
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

static const char *TEST_DB_PATH = "/tmp/test_mimi_session.db";

/* Test session create and delete */
TEST(test_session_create_delete)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Create session */
    ASSERT(mimi_mem_session_create(pool, "test-session-1") == MIMI_MEMORY_OK);
    
    /* Create duplicate */
    int ret = mimi_mem_session_create(pool, "test-session-1");
    ASSERT(ret == MIMI_MEMORY_ERR_EXISTS);
    
    /* Delete session */
    ASSERT(mimi_mem_session_delete(pool, "test-session-1") == MIMI_MEMORY_OK);
    
    /* Delete non-existent */
    ret = mimi_mem_session_delete(pool, "test-session-1");
    ASSERT(ret == MIMI_MEMORY_ERR_NOT_FOUND);
    
    mimi_mem_pool_destroy(pool);
}

/* Test session exists check */
TEST(test_session_exists)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Check non-existent */
    ASSERT(mimi_mem_session_exists(pool, "nonexistent") == 0);
    
    /* Create and check */
    ASSERT(mimi_mem_session_create(pool, "exists-test") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_exists(pool, "exists-test") == 1);
    
    /* Delete and check */
    ASSERT(mimi_mem_session_delete(pool, "exists-test") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_exists(pool, "exists-test") == 0);
    
    mimi_mem_pool_destroy(pool);
}

/* Test session list */
TEST(test_session_list)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Initially empty */
    char **sessions = NULL;
    int count = 0;
    ASSERT(mimi_mem_session_list(pool, &sessions, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 0);
    
    /* Create sessions */
    ASSERT(mimi_mem_session_create(pool, "list-1") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_create(pool, "list-2") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_create(pool, "list-3") == MIMI_MEMORY_OK);
    
    /* List sessions */
    ASSERT(mimi_mem_session_list(pool, &sessions, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 3);
    
    /* Free session list */
    for (int i = 0; i < count; i++) {
        free(sessions[i]);
    }
    free(sessions);
    
    /* Cleanup */
    ASSERT(mimi_mem_session_delete(pool, "list-1") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_delete(pool, "list-2") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_delete(pool, "list-3") == MIMI_MEMORY_OK);
    
    mimi_mem_pool_destroy(pool);
}

/* Test message append and query */
TEST(test_session_message_append_query)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    ASSERT(mimi_mem_session_create(pool, "msg-test") == MIMI_MEMORY_OK);
    
    /* Append messages */
    int64_t msg_id1, msg_id2, msg_id3;
    ASSERT(mimi_mem_message_append(pool, "msg-test", "user", "Hello!", &msg_id1) == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_message_append(pool, "msg-test", "assistant", "Hi there!", &msg_id2) == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_message_append(pool, "msg-test", "user", "How are you?", &msg_id3) == MIMI_MEMORY_OK);
    
    /* Verify message IDs are generated */
    ASSERT(msg_id1 > 0);
    ASSERT(msg_id2 > 0);
    ASSERT(msg_id3 > 0);
    
    /* Query messages */
    mimi_mem_message_t **messages = NULL;
    int count = 0;
    ASSERT(mimi_mem_message_query(pool, "msg-test", 0, &messages, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 3);
    
    /* Verify messages */
    ASSERT(strcmp(messages[0]->role, "user") == 0);
    ASSERT(strcmp(messages[0]->content, "Hello!") == 0);
    ASSERT(strcmp(messages[1]->role, "assistant") == 0);
    ASSERT(strcmp(messages[1]->content, "Hi there!") == 0);
    ASSERT(strcmp(messages[2]->role, "user") == 0);
    ASSERT(strcmp(messages[2]->content, "How are you?") == 0);
    
    /* Free messages */
    for (int i = 0; i < count; i++) {
        mimi_mem_message_free(messages[i]);
    }
    free(messages);
    
    ASSERT(mimi_mem_session_delete(pool, "msg-test") == MIMI_MEMORY_OK);
    mimi_mem_pool_destroy(pool);
}

/* Test message query with limit */
TEST(test_session_message_query_limit)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    ASSERT(mimi_mem_session_create(pool, "limit-test") == MIMI_MEMORY_OK);
    
    /* Append 10 messages */
    for (int i = 0; i < 10; i++) {
        char content[64];
        snprintf(content, sizeof(content), "Message %d", i);
        ASSERT(mimi_mem_message_append(pool, "limit-test", "user", content, NULL) == MIMI_MEMORY_OK);
    }
    
    /* Query with limit 5 */
    mimi_mem_message_t **messages = NULL;
    int count = 0;
    ASSERT(mimi_mem_message_query(pool, "limit-test", 5, &messages, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 5);
    
    /* Free messages */
    for (int i = 0; i < count; i++) {
        mimi_mem_message_free(messages[i]);
    }
    free(messages);
    
    /* Query all */
    messages = NULL;
    count = 0;
    ASSERT(mimi_mem_message_query(pool, "limit-test", 0, &messages, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 10);
    
    for (int i = 0; i < count; i++) {
        mimi_mem_message_free(messages[i]);
    }
    free(messages);
    
    ASSERT(mimi_mem_session_delete(pool, "limit-test") == MIMI_MEMORY_OK);
    mimi_mem_pool_destroy(pool);
}

/* Test message clear */
TEST(test_session_message_clear)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    ASSERT(mimi_mem_session_create(pool, "clear-test") == MIMI_MEMORY_OK);
    
    /* Append messages */
    for (int i = 0; i < 5; i++) {
        ASSERT(mimi_mem_message_append(pool, "clear-test", "user", "test", NULL) == MIMI_MEMORY_OK);
    }
    
    /* Verify messages exist */
    mimi_mem_message_t **messages = NULL;
    int count = 0;
    ASSERT(mimi_mem_message_query(pool, "clear-test", 0, &messages, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 5);
    
    for (int i = 0; i < count; i++) {
        mimi_mem_message_free(messages[i]);
    }
    free(messages);
    
    /* Clear messages */
    ASSERT(mimi_mem_message_clear(pool, "clear-test") == MIMI_MEMORY_OK);
    
    /* Verify cleared */
    messages = NULL;
    count = 0;
    ASSERT(mimi_mem_message_query(pool, "clear-test", 0, &messages, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 0);
    
    ASSERT(mimi_mem_session_delete(pool, "clear-test") == MIMI_MEMORY_OK);
    mimi_mem_pool_destroy(pool);
}

/* Test search functionality */
TEST(test_session_search)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Write entries for search */
    mimi_mem_write(pool, "search_apple", "fruit");
    mimi_mem_write(pool, "search_banana", "fruit");
    mimi_mem_write(pool, "search_carrot", "vegetable");
    mimi_mem_write(pool, "search_dog", "animal");
    
    /* Search for "fruit" */
    mimi_mem_entry_t **results = NULL;
    int count = 0;
    ASSERT(mimi_mem_search(pool, "fruit", &results, &count) == MIMI_MEMORY_OK);
    ASSERT(count >= 2);  /* At least apple and banana */
    
    /* Free results */
    for (int i = 0; i < count; i++) {
        mimi_mem_entry_free(results[i]);
    }
    free(results);
    
    /* Search with no results */
    results = NULL;
    count = 0;
    ASSERT(mimi_mem_search(pool, "nonexistent_xyz", &results, &count) == MIMI_MEMORY_OK);
    ASSERT(count == 0);
    
    mimi_mem_pool_destroy(pool);
}

/* Test session isolation */
TEST(test_session_isolation)
{
    mimi_mem_config_t config = {
        .db_path = TEST_DB_PATH,
        .pool_size = 2,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    ASSERT(pool != NULL);
    
    /* Create two sessions */
    ASSERT(mimi_mem_session_create(pool, "session-A") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_create(pool, "session-B") == MIMI_MEMORY_OK);
    
    /* Add messages to session A */
    ASSERT(mimi_mem_message_append(pool, "session-A", "user", "Message for A", NULL) == MIMI_MEMORY_OK);
    
    /* Add messages to session B */
    ASSERT(mimi_mem_message_append(pool, "session-B", "user", "Message for B", NULL) == MIMI_MEMORY_OK);
    
    /* Verify isolation */
    mimi_mem_message_t **messages_a = NULL;
    int count_a = 0;
    ASSERT(mimi_mem_message_query(pool, "session-A", 0, &messages_a, &count_a) == MIMI_MEMORY_OK);
    ASSERT(count_a == 1);
    ASSERT(strcmp(messages_a[0]->content, "Message for A") == 0);
    
    mimi_mem_message_t **messages_b = NULL;
    int count_b = 0;
    ASSERT(mimi_mem_message_query(pool, "session-B", 0, &messages_b, &count_b) == MIMI_MEMORY_OK);
    ASSERT(count_b == 1);
    ASSERT(strcmp(messages_b[0]->content, "Message for B") == 0);
    
    /* Free messages */
    for (int i = 0; i < count_a; i++) mimi_mem_message_free(messages_a[i]);
    for (int i = 0; i < count_b; i++) mimi_mem_message_free(messages_b[i]);
    free(messages_a);
    free(messages_b);
    
    ASSERT(mimi_mem_session_delete(pool, "session-A") == MIMI_MEMORY_OK);
    ASSERT(mimi_mem_session_delete(pool, "session-B") == MIMI_MEMORY_OK);
    mimi_mem_pool_destroy(pool);
}

int main(void)
{
    printf("=== libmimi-memory Session Tests ===\n\n");
    
    RUN_TEST(test_session_create_delete);
    RUN_TEST(test_session_exists);
    RUN_TEST(test_session_list);
    RUN_TEST(test_session_message_append_query);
    RUN_TEST(test_session_message_query_limit);
    RUN_TEST(test_session_message_clear);
    RUN_TEST(test_session_search);
    RUN_TEST(test_session_isolation);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    /* Cleanup test database */
    remove(TEST_DB_PATH);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
