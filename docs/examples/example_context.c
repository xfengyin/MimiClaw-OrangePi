/**
 * @file example_context.c
 * @brief Context management example
 * 
 * Demonstrates:
 * - Session creation and management
 * - Context storage and retrieval
 * - Context isolation between sessions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mimi_core.h"

int main(void)
{
    printf("=== MimiClaw Context Management Example ===\n\n");
    
    /* Configure core */
    mimi_core_config_t config = {
        .api_key = "your-api-key-here",
        .model = "claude-3-5-sonnet",
        .max_tokens = 500,
        .temperature = 0.5f,
        .timeout_ms = 30000
    };
    
    /* Initialize */
    mimi_core_ctx_t *ctx = NULL;
    if (mimi_core_init(&ctx, &config) != MIMI_CORE_OK) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }
    
    /* Create two sessions */
    printf("Creating session 'alice'...\n");
    mimi_core_session_create(ctx, "alice");
    
    printf("Creating session 'bob'...\n");
    mimi_core_session_create(ctx, "bob");
    
    /* Set context for Alice */
    printf("\nSetting context for Alice:\n");
    mimi_core_set_context(ctx, "alice", "name", "Alice");
    mimi_core_set_context(ctx, "alice", "preference", "Python");
    mimi_core_set_context(ctx, "alice", "level", "Advanced");
    printf("  - name: Alice\n");
    printf("  - preference: Python\n");
    printf("  - level: Advanced\n");
    
    /* Set context for Bob */
    printf("\nSetting context for Bob:\n");
    mimi_core_set_context(ctx, "bob", "name", "Bob");
    mimi_core_set_context(ctx, "bob", "preference", "C");
    mimi_core_set_context(ctx, "bob", "level", "Beginner");
    printf("  - name: Bob\n");
    printf("  - preference: C\n");
    printf("  - level: Beginner\n");
    
    /* Retrieve and verify Alice's context */
    printf("\n--- Retrieving Alice's Context ---\n");
    char *value = NULL;
    
    if (mimi_core_get_context(ctx, "alice", "name", &value) == MIMI_CORE_OK) {
        printf("Alice's name: %s\n", value);
        free(value);
    }
    
    if (mimi_core_get_context(ctx, "alice", "preference", &value) == MIMI_CORE_OK) {
        printf("Alice's preference: %s\n", value);
        free(value);
    }
    
    /* Retrieve and verify Bob's context */
    printf("\n--- Retrieving Bob's Context ---\n");
    
    if (mimi_core_get_context(ctx, "bob", "name", &value) == MIMI_CORE_OK) {
        printf("Bob's name: %s\n", value);
        free(value);
    }
    
    if (mimi_core_get_context(ctx, "bob", "preference", &value) == MIMI_CORE_OK) {
        printf("Bob's preference: %s\n", value);
        free(value);
    }
    
    /* Verify isolation - Bob should NOT have Alice's data */
    printf("\n--- Verifying Session Isolation ---\n");
    if (mimi_core_get_context(ctx, "bob", "nonexistent", &value) != MIMI_CORE_OK) {
        printf("✓ Session isolation verified (Bob doesn't have Alice's data)\n");
    }
    
    /* Update context */
    printf("\n--- Updating Alice's Level ---\n");
    mimi_core_set_context(ctx, "alice", "level", "Expert");
    
    if (mimi_core_get_context(ctx, "alice", "level", &value) == MIMI_CORE_OK) {
        printf("Alice's updated level: %s\n", value);
        free(value);
    }
    
    /* Delete specific context */
    printf("\n--- Deleting Alice's Preference ---\n");
    mimi_core_delete_context(ctx, "alice", "preference");
    
    if (mimi_core_get_context(ctx, "alice", "preference", &value) != MIMI_CORE_OK) {
        printf("✓ Preference deleted successfully\n");
    }
    
    /* Clear all context for a session */
    printf("\n--- Clearing All of Bob's Context ---\n");
    mimi_core_clear_context(ctx, "bob");
    
    if (mimi_core_get_context(ctx, "bob", "name", &value) != MIMI_CORE_OK) {
        printf("✓ All context cleared for Bob\n");
    }
    
    /* List sessions */
    printf("\n--- Active Sessions ---\n");
    char **sessions = NULL;
    int count = 0;
    if (mimi_core_session_list(ctx, &sessions, &count) == MIMI_CORE_OK) {
        printf("Total sessions: %d\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - %s\n", sessions[i]);
            free(sessions[i]);
        }
        free(sessions);
    }
    
    /* Delete sessions */
    printf("\n--- Cleaning Up ---\n");
    mimi_core_session_delete(ctx, "alice");
    mimi_core_session_delete(ctx, "bob");
    printf("Sessions deleted\n");
    
    /* Destroy core */
    mimi_core_destroy(ctx);
    printf("\nDone!\n");
    
    return 0;
}

/*
 * Compile:
 * gcc -I../../libs/libmimi-core/include \
 *     -I../../libs/libmimi-memory/include \
 *     example_context.c \
 *     -L../../libs/build/lib \
 *     -lmimi-core -lmimi-memory -lsqlite3 -lpthread \
 *     -o example_context
 *
 * Run:
 * ./example_context
 */
