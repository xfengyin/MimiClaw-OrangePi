/**
 * @file test_core.c
 * @brief Unit tests for libmimi-core
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

TEST(test_version)
{
    const char *version = mimi_core_version();
    ASSERT(version != NULL);
    ASSERT(strcmp(version, "2.0.0") == 0);
}

TEST(test_init_destroy)
{
    mimi_core_config_t config = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx = NULL;
    int ret = mimi_core_init(&ctx, &config);
    ASSERT(ret == MIMI_CORE_OK);
    ASSERT(ctx != NULL);
    
    ret = mimi_core_destroy(ctx);
    ASSERT(ret == MIMI_CORE_OK);
}

TEST(test_init_invalid_config)
{
    mimi_core_config_t config = {0}; /* Invalid: empty api_key */
    mimi_core_ctx_t *ctx = NULL;
    int ret = mimi_core_init(&ctx, &config);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG);
    ASSERT(ctx == NULL);
}

TEST(test_init_null_args)
{
    mimi_core_config_t config = {
        .api_key = "test",
        .model = "test",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    /* NULL ctx pointer */
    int ret = mimi_core_init(NULL, &config);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG);
    
    /* NULL config pointer */
    mimi_core_ctx_t *ctx = NULL;
    ret = mimi_core_init(&ctx, NULL);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG);
}

TEST(test_session_create_delete)
{
    mimi_core_config_t config = {
        .api_key = "test",
        .model = "test",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    
    /* Create session */
    int ret = mimi_core_session_create(ctx, "test-session");
    ASSERT(ret == MIMI_CORE_OK);
    
    /* Create duplicate */
    ret = mimi_core_session_create(ctx, "test-session");
    ASSERT(ret == MIMI_CORE_ERR_SESSION_EXISTS);
    
    /* Delete session */
    ret = mimi_core_session_delete(ctx, "test-session");
    ASSERT(ret == MIMI_CORE_OK);
    
    /* Delete non-existent */
    ret = mimi_core_session_delete(ctx, "test-session");
    ASSERT(ret == MIMI_CORE_ERR_SESSION_NOT_FOUND);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

TEST(test_context_set_get)
{
    mimi_core_config_t config = {
        .api_key = "test",
        .model = "test",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    ASSERT(mimi_core_session_create(ctx, "test") == MIMI_CORE_OK);
    
    /* Set context */
    int ret = mimi_core_set_context(ctx, "test", "key1", "value1");
    ASSERT(ret == MIMI_CORE_OK);
    
    /* Get context */
    char *value;
    ret = mimi_core_get_context(ctx, "test", "key1", &value);
    ASSERT(ret == MIMI_CORE_OK);
    ASSERT(strcmp(value, "value1") == 0);
    free(value);
    
    /* Get non-existent key */
    ret = mimi_core_get_context(ctx, "test", "nonexistent", &value);
    ASSERT(ret == MIMI_CORE_ERR_NOT_FOUND);
    
    /* Update context */
    ret = mimi_core_set_context(ctx, "test", "key1", "value2");
    ASSERT(ret == MIMI_CORE_OK);
    
    ret = mimi_core_get_context(ctx, "test", "key1", &value);
    ASSERT(ret == MIMI_CORE_OK);
    ASSERT(strcmp(value, "value2") == 0);
    free(value);
    
    /* Delete context */
    ret = mimi_core_delete_context(ctx, "test", "key1");
    ASSERT(ret == MIMI_CORE_OK);
    
    ret = mimi_core_get_context(ctx, "test", "key1", &value);
    ASSERT(ret == MIMI_CORE_ERR_NOT_FOUND);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

TEST(test_chat)
{
    mimi_core_config_t config = {
        .api_key = "test",
        .model = "test",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 5000
    };
    
    mimi_core_ctx_t *ctx;
    ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
    
    mimi_core_response_t response;
    int ret = mimi_core_chat(ctx, "session-1", "Hello!", &response);
    ASSERT(ret == MIMI_CORE_OK);
    ASSERT(response.content != NULL);
    ASSERT(response.finish_reason != NULL);
    
    mimi_core_response_free(&response);
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

TEST(test_strerror)
{
    ASSERT(mimi_core_strerror(MIMI_CORE_OK) != NULL);
    ASSERT(mimi_core_strerror(MIMI_CORE_ERR_INVALID_ARG) != NULL);
    ASSERT(mimi_core_strerror(-999) != NULL); /* Unknown error */
}

int main(void)
{
    printf("=== libmimi-core Unit Tests ===\n\n");
    
    RUN_TEST(test_version);
    RUN_TEST(test_init_destroy);
    RUN_TEST(test_init_invalid_config);
    RUN_TEST(test_init_null_args);
    RUN_TEST(test_session_create_delete);
    RUN_TEST(test_context_set_get);
    RUN_TEST(test_chat);
    RUN_TEST(test_strerror);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
