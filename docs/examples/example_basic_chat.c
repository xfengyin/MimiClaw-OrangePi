/**
 * @file example_basic_chat.c
 * @brief Basic chat example using MimiClaw Core
 * 
 * Demonstrates:
 * - Core initialization
 * - Simple chat conversation
 * - Response handling
 */

#include <stdio.h>
#include <stdlib.h>
#include "mimi_core.h"

int main(void)
{
    printf("=== MimiClaw Basic Chat Example ===\n\n");
    
    /* Configure core */
    mimi_core_config_t config = {
        .api_key = "your-api-key-here",
        .model = "claude-3-5-sonnet",
        .max_tokens = 1000,
        .temperature = 0.7f,
        .timeout_ms = 30000
    };
    
    /* Initialize core context */
    mimi_core_ctx_t *ctx = NULL;
    int ret = mimi_core_init(&ctx, &config);
    if (ret != MIMI_CORE_OK) {
        fprintf(stderr, "Failed to initialize core: %s\n", mimi_core_strerror(ret));
        return 1;
    }
    
    printf("Core initialized successfully\n\n");
    
    /* First message */
    printf("User: Hello! What can you help me with?\n");
    
    mimi_core_response_t response;
    ret = mimi_core_chat(ctx, "session-1", "Hello! What can you help me with?", &response);
    if (ret != MIMI_CORE_OK) {
        fprintf(stderr, "Chat failed: %s\n", mimi_core_strerror(ret));
        mimi_core_destroy(ctx);
        return 1;
    }
    
    printf("AI: %s\n\n", response.content);
    printf("Tokens used: prompt=%d, completion=%d\n\n", 
           response.usage_prompt_tokens, response.usage_completion_tokens);
    
    /* Free response */
    mimi_core_response_free(&response);
    
    /* Second message (continues conversation) */
    printf("User: Can you write a simple C function?\n");
    
    ret = mimi_core_chat(ctx, "session-1", "Can you write a simple C function?", &response);
    if (ret != MIMI_CORE_OK) {
        fprintf(stderr, "Chat failed: %s\n", mimi_core_strerror(ret));
        mimi_core_destroy(ctx);
        return 1;
    }
    
    printf("AI: %s\n\n", response.content);
    
    /* Free response */
    mimi_core_response_free(&response);
    
    /* Cleanup */
    mimi_core_destroy(ctx);
    printf("Core destroyed. Goodbye!\n");
    
    return 0;
}

/*
 * Compile:
 * gcc -I../../libs/libmimi-core/include \
 *     -I../../libs/libmimi-memory/include \
 *     example_basic_chat.c \
 *     -L../../libs/build/lib \
 *     -lmimi-core -lmimi-memory -lsqlite3 -lpthread \
 *     -o example_basic_chat
 *
 * Run:
 * ./example_basic_chat
 */
