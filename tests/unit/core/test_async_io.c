/**
 * @file test_async_io.c
 * @brief Unit tests for Async I/O functionality in libmimi-core
 * @coverage mimi_core async operations, timeout handling, response parsing
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

/* Test chat response structure */
TEST(test_async_response_structure)
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
    
    mimi_core_response_t response = {0};
    int ret = mimi_core_chat(ctx, "async-session-1", "Test message", &response);
    ASSERT(ret == MIMI_CORE_OK);
    
    /* Verify response structure */
    ASSERT(response.content != NULL);
    ASSERT(response.finish_reason != NULL);
    ASSERT(response.usage_prompt_tokens >= 0);
    ASSERT(response.usage_completion_tokens >= 0);
    
    /* Tool results may be NULL if no tools were called */
    if (response.tool_results_count > 0) {
        ASSERT(response.tool_results != NULL);
    }
    
    mimi_core_response_free(&response);
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test chat with different message types */
TEST(test_async_various_messages)
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
    
    const char *messages[] = {
        "Hello",
        "What is 2+2?",
        "Write a short poem",
        "Explain quantum computing",
        "",  /* Empty message */
        "Special chars: !@#$%^&*()",
        "Unicode: 你好世界 🌍"
    };
    
    for (size_t i = 0; i < sizeof(messages)/sizeof(messages[0]); i++) {
        mimi_core_response_t response = {0};
        int ret = mimi_core_chat(ctx, "async-session-2", messages[i], &response);
        ASSERT(ret == MIMI_CORE_OK);
        ASSERT(response.content != NULL);
        mimi_core_response_free(&response);
    }
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test conversation history continuity */
TEST(test_async_conversation_history)
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
    
    /* First message */
    mimi_core_response_t response1 = {0};
    int ret = mimi_core_chat(ctx, "conversation-test", "My name is Alice", &response1);
    ASSERT(ret == MIMI_CORE_OK);
    mimi_core_response_free(&response1);
    
    /* Second message should remember context */
    mimi_core_response_t response2 = {0};
    ret = mimi_core_chat(ctx, "conversation-test", "What is my name?", &response2);
    ASSERT(ret == MIMI_CORE_OK);
    ASSERT(response2.content != NULL);
    /* Response should mention "Alice" */
    ASSERT(strstr(response2.content, "Alice") != NULL || 
           strstr(response2.content, "alice") != NULL);
    mimi_core_response_free(&response2);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

/* Test timeout configuration */
TEST(test_async_timeout_config)
{
    /* Test with very short timeout */
    mimi_core_config_t config_short = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 100  /* Very short timeout */
    };
    
    mimi_core_ctx_t *ctx_short = NULL;
    int ret = mimi_core_init(&ctx_short, &config_short);
    ASSERT(ret == MIMI_CORE_OK);
    
    mimi_core_response_t response = {0};
    ret = mimi_core_chat(ctx_short, "timeout-test", "Hello", &response);
    /* May succeed or timeout depending on implementation */
    ASSERT(ret == MIMI_CORE_OK || ret == MIMI_CORE_ERR_TIMEOUT);
    
    if (response.content != NULL) {
        mimi_core_response_free(&response);
    }
    ASSERT(mimi_core_destroy(ctx_short) == MIMI_CORE_OK);
    
    /* Test with normal timeout */
    mimi_core_config_t config_normal = {
        .api_key = "test-key",
        .model = "test-model",
        .max_tokens = 100,
        .temperature = 0.5f,
        .timeout_ms = 30000
    };
    
    mimi_core_ctx_t *ctx_normal = NULL;
    ASSERT(mimi_core_init(&ctx_normal, &config_normal) == MIMI_CORE_OK);
    ASSERT(mimi_core_destroy(ctx_normal) == MIMI_CORE_OK);
}

/* Test temperature settings */
TEST(test_async_temperature_settings)
{
    float temperatures[] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
    
    for (size_t i = 0; i < sizeof(temperatures)/sizeof(temperatures[0]); i++) {
        mimi_core_config_t config = {
            .api_key = "test-key",
            .model = "test-model",
            .max_tokens = 100,
            .temperature = temperatures[i],
            .timeout_ms = 5000
        };
        
        mimi_core_ctx_t *ctx = NULL;
        ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
        
        mimi_core_response_t response = {0};
        int ret = mimi_core_chat(ctx, "temp-test", "Say hello", &response);
        ASSERT(ret == MIMI_CORE_OK);
        ASSERT(response.content != NULL);
        mimi_core_response_free(&response);
        
        ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
    }
}

/* Test max_tokens configuration */
TEST(test_async_max_tokens)
{
    int max_tokens[] = {10, 50, 100, 500};
    
    for (size_t i = 0; i < sizeof(max_tokens)/sizeof(max_tokens[0]); i++) {
        mimi_core_config_t config = {
            .api_key = "test-key",
            .model = "test-model",
            .max_tokens = max_tokens[i],
            .temperature = 0.5f,
            .timeout_ms = 5000
        };
        
        mimi_core_ctx_t *ctx = NULL;
        ASSERT(mimi_core_init(&ctx, &config) == MIMI_CORE_OK);
        
        mimi_core_response_t response = {0};
        int ret = mimi_core_chat(ctx, "tokens-test", "Write a story", &response);
        ASSERT(ret == MIMI_CORE_OK);
        ASSERT(response.content != NULL);
        
        /* Verify usage tokens don't exceed max */
        ASSERT(response.usage_completion_tokens <= max_tokens[i] + 10); /* Some tolerance */
        
        mimi_core_response_free(&response);
        ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
    }
}

/* Test response free with NULL fields */
TEST(test_async_response_free_safety)
{
    /* Test freeing empty response */
    mimi_core_response_t empty_response = {0};
    mimi_core_response_free(&empty_response);
    
    /* Test freeing response with only content */
    mimi_core_response_t partial_response = {0};
    partial_response.content = strdup("test");
    partial_response.finish_reason = NULL;
    mimi_core_response_free(&partial_response);
    
    /* Test freeing response with all fields */
    mimi_core_response_t full_response = {0};
    full_response.content = strdup("test content");
    full_response.finish_reason = strdup("stop");
    full_response.usage_prompt_tokens = 10;
    full_response.usage_completion_tokens = 20;
    mimi_core_response_free(&full_response);
}

/* Test error handling in chat */
TEST(test_async_error_handling)
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
    
    /* Test with NULL session_id */
    mimi_core_response_t response = {0};
    int ret = mimi_core_chat(ctx, NULL, "Hello", &response);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG || ret == MIMI_CORE_OK);
    
    /* Test with NULL response pointer */
    ret = mimi_core_chat(ctx, "test", "Hello", NULL);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG);
    
    /* Test with NULL message */
    ret = mimi_core_chat(ctx, "test", NULL, &response);
    ASSERT(ret == MIMI_CORE_ERR_INVALID_ARG || ret == MIMI_CORE_OK);
    
    ASSERT(mimi_core_destroy(ctx) == MIMI_CORE_OK);
}

int main(void)
{
    printf("=== libmimi-core Async I/O Tests ===\n\n");
    
    RUN_TEST(test_async_response_structure);
    RUN_TEST(test_async_various_messages);
    RUN_TEST(test_async_conversation_history);
    RUN_TEST(test_async_timeout_config);
    RUN_TEST(test_async_temperature_settings);
    RUN_TEST(test_async_max_tokens);
    RUN_TEST(test_async_response_free_safety);
    RUN_TEST(test_async_error_handling);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
