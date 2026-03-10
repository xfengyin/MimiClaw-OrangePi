/**
 * @file example_core.c
 * @brief Example: Using libmimi-core
 */

#include <stdio.h>
#include <stdlib.h>
#include "mimi_core.h"

int main(void)
{
    printf("=== MimiCore Example ===\n\n");
    printf("Library version: %s\n\n", mimi_core_version());
    
    /* Initialize */
    mimi_core_config_t config = {
        .api_key = "test-api-key",
        .model = "claude-3-5-sonnet",
        .max_tokens = 1024,
        .temperature = 0.7f,
        .timeout_ms = 30000
    };
    
    mimi_core_ctx_t *ctx;
    int ret = mimi_core_init(&ctx, &config);
    if (ret != MIMI_CORE_OK) {
        fprintf(stderr, "Init failed: %s\n", mimi_core_strerror(ret));
        return 1;
    }
    printf("✓ Core initialized\n");
    
    /* Create session */
    ret = mimi_core_session_create(ctx, "test-session");
    if (ret != MIMI_CORE_OK && ret != MIMI_CORE_ERR_SESSION_EXISTS) {
        fprintf(stderr, "Session create failed: %s\n", mimi_core_strerror(ret));
        mimi_core_destroy(ctx);
        return 1;
    }
    printf("✓ Session created\n");
    
    /* Set context */
    ret = mimi_core_set_context(ctx, "test-session", "user_name", "Alice");
    if (ret != MIMI_CORE_OK) {
        fprintf(stderr, "Set context failed: %s\n", mimi_core_strerror(ret));
    } else {
        printf("✓ Context set\n");
    }
    
    /* Get context */
    char *value;
    ret = mimi_core_get_context(ctx, "test-session", "user_name", &value);
    if (ret == MIMI_CORE_OK) {
        printf("✓ Context retrieved: user_name = %s\n", value);
        free(value);
    }
    
    /* Chat (placeholder) */
    mimi_core_response_t response;
    ret = mimi_core_chat(ctx, "test-session", "Hello!", &response);
    if (ret == MIMI_CORE_OK) {
        printf("✓ Chat response: %s\n", response.content);
        mimi_core_response_free(&response);
    }
    
    /* List sessions */
    char **sessions;
    int count;
    ret = mimi_core_session_list(ctx, &sessions, &count);
    if (ret == MIMI_CORE_OK) {
        printf("✓ Sessions (%d):\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - %s\n", sessions[i]);
            free(sessions[i]);
        }
        free(sessions);
    }
    
    /* Cleanup */
    mimi_core_destroy(ctx);
    printf("✓ Core destroyed\n");
    
    printf("\n=== Example Complete ===\n");
    return 0;
}
