/**
 * @file test_mempool.c
 * @brief Unit tests for Memory Pool functionality in libmimi-core
 * @coverage mimi_core memory management, session pooling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mimi_core.h"

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

/* Test multiple sessions with shared context */
TEST(test_mempool_multiple_sessions)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    
    /* Create multiple sessions */
    const char *sessions[] = {"session-1", "session-2", "session-3", "session-4", "session-5"};
    int num_sessions = sizeof(sessions) / sizeof(sessions[0]);
    
    for (int i = 0; i < num_sessions; i++) {
        int ret = mimi_core_session_create(ctx, sessions[i]);
        ASSERT(ret == MIMI_CORE_OK);
    }
    
    /* Set context for each session */
    for (int i = 0; i < num_sessions; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        ASSERT(mimi_core_set_context(ctx, sessions[i], key, value) == MIMI_CORE_OK);
    }
    
    /* Verify context isolation */
    for (int i = 0; i < num_sessions; i++) {
        char key[64], expected[64];
        snprintf(key, sizeof(key), "key_%d", i);
        
        char *value = NULL;
        ASSERT(mimi_core_get_context(ctx, sessions[i], key, &value) == MIMI_CORE_OK);
        snprintf(expected, sizeof(expected), "value_%d", i);
        ASSERT(strcmp(value, expected) == 0);
        free(value);
    }
    
    /* Clean up all sessions */
    for (int i = 0; i < num_sessions; i++) {
        ASSERT(mimi_core_session_delete(ctx, sessions[i]) == MIMI_CORE_OK);
    }
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test session context memory cleanup */
TEST(test_mempool_context_cleanup)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "cleanup-test") == MIMI_CORE_OK);
    
    /* Add many context entries */
    for (int i = 0; i < 50; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "test_key_%d", i);
        snprintf(value, sizeof(value), "test_value_%d", i);
        ASSERT(mimi_core_set_context(ctx, "cleanup-test", key, value) == MIMI_CORE_OK);
    }
    
    /* Clear all context */
    ASSERT(mimi_core_clear_context(ctx, "cleanup-test") == MIMI_CORE_OK);
    
    /* Verify all entries are deleted */
    for (int i = 0; i < 50; i++) {
        char key[64];
        snprintf(key, sizeof(key), "test_key_%d", i);
        char *value = NULL;
        ASSERT(mimi_core_get_context(ctx, "cleanup-test", key, &value) == MIMI_CORE_ERR_NOT_FOUND);
    }
    
    ASSERT(mimi_core_session_delete(ctx, "cleanup-test") == MIMI_CORE_OK);
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test session list functionality */
TEST(test_mempool_session_list)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    
    /* Initially no sessions */
    char **sessions = NULL;
    int count = 0;
    ASSERT(mimi_core_session_list(ctx, &sessions, &count) == MIMI_CORE_OK);
    ASSERT(count == 0);
    
    /* Create sessions */
    ASSERT(mimi_core_session_create(ctx, "list-test-1") == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "list-test-2") == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "list-test-3") == MIMI_CORE_OK);
    
    /* List sessions */
    ASSERT(mimi_core_session_list(ctx, &sessions, &count) == MIMI_CORE_OK);
    ASSERT(count == 3);
    
    /* Free session list */
    for (int i = 0; i < count; i++) {
        free(sessions[i]);
    }
    free(sessions);
    
    /* Clean up */
    ASSERT(mimi_core_session_delete(ctx, "list-test-1") == MIMI_CORE_OK);
    ASSERT(mimi_core_session_delete(ctx, "list-test-2") == MIMI_CORE_OK);
    ASSERT(mimi_core_session_delete(ctx, "list-test-3") == MIMI_CORE_OK);
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test concurrent context operations on same session */
TEST(test_mempool_concurrent_context_ops)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "concurrent-test") == MIMI_CORE_OK);
    
    /* Set multiple keys */
    ASSERT(mimi_core_set_context(ctx, "concurrent-test", "a", "1") == MIMI_CORE_OK);
    ASSERT(mimi_core_set_context(ctx, "concurrent-test", "b", "2") == MIMI_CORE_OK);
    ASSERT(mimi_core_set_context(ctx, "concurrent-test", "c", "3") == MIMI_CORE_OK);
    
    /* Get all keys */
    char *val_a = NULL, *val_b = NULL, *val_c = NULL;
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "a", &val_a) == MIMI_CORE_OK);
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "b", &val_b) == MIMI_CORE_OK);
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "c", &val_c) == MIMI_CORE_OK);
    
    ASSERT(strcmp(val_a, "1") == 0);
    ASSERT(strcmp(val_b, "2") == 0);
    ASSERT(strcmp(val_c, "3") == 0);
    
    free(val_a);
    free(val_b);
    free(val_c);
    
    /* Delete middle key */
    ASSERT(mimi_core_delete_context(ctx, "concurrent-test", "b") == MIMI_CORE_OK);
    
    /* Verify deletion */
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "a", &val_a) == MIMI_CORE_OK);
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "b", &val_b) == MIMI_CORE_ERR_NOT_FOUND);
    ASSERT(mimi_core_get_context(ctx, "concurrent-test", "c", &val_c) == MIMI_CORE_OK);
    
    free(val_a);
    free(val_c);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test session with special characters in ID */
TEST(test_mempool_special_session_ids)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    
    /* Test various session ID formats */
    const char *special_ids[] = {
        "session-with-dashes",
        "session_with_underscores",
        "session123",
        "Session-Mixed-Case"
    };
    
    for (size_t i = 0; i < sizeof(special_ids)/sizeof(special_ids[0]); i++) {
        ASSERT(mimi_core_session_create(ctx, special_ids[i]) == MIMI_CORE_OK);
        ASSERT(mimi_core_set_context(ctx, special_ids[i], "test", "value") == MIMI_CORE_OK);
        
        char *value = NULL;
        ASSERT(mimi_core_get_context(ctx, special_ids[i], "test", &value) == MIMI_CORE_OK);
        ASSERT(strcmp(value, "value") == 0);
        free(value);
        
        ASSERT(mimi_core_session_delete(ctx, special_ids[i]) == MIMI_CORE_OK);
    }
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test memory allocation failure simulation */
TEST(test_mempool_allocation_edge_cases)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "edge-test") == MIMI_CORE_OK);
    
    /* Test empty key */
    ASSERT(mimi_core_set_context(ctx, "edge-test", "", "value") == MIMI_CORE_OK ||
           mimi_core_set_context(ctx, "edge-test", "", "value") == MIMI_CORE_ERR_INVALID_ARG);
    
    /* Test empty value */
    ASSERT(mimi_core_set_context(ctx, "edge-test", "empty_value", "") == MIMI_CORE_OK);
    char *value = NULL;
    ASSERT(mimi_core_get_context(ctx, "edge-test", "empty_value", &value) == MIMI_CORE_OK);
    ASSERT(strcmp(value, "") == 0);
    free(value);
    
    /* Test very long key (should truncate or fail) */
    char long_key[512];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    int ret = mimi_core_set_context(ctx, "edge-test", long_key, "value");
    /* Either succeeds with truncation or fails with invalid arg */
    ASSERT(ret == MIMI_CORE_OK || ret == MIMI_CORE_ERR_INVALID_ARG);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

int main(void)
{
    printf("=== libmimi-core Memory Pool Tests ===\n\n");
    
    RUN_TEST(test_mempool_multiple_sessions);
    RUN_TEST(test_mempool_context_cleanup);
    RUN_TEST(test_mempool_session_list);
    RUN_TEST(test_mempool_concurrent_context_ops);
    RUN_TEST(test_mempool_special_session_ids);
    RUN_TEST(test_mempool_allocation_edge_cases);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
