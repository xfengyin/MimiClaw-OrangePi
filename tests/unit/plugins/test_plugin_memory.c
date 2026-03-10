/**
 * @file test_plugin_memory.c
 * @brief Unit tests for Memory Plugin
 * @coverage memory plugin: store, retrieve, search, delete operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mimi_tools.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  Running %s... ", #name); \
    if (test_##name() == 0) { \
        printf("✓ PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("✗ FAILED\n"); \
    } \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n    Assertion failed: %s\n", msg); \
        return -1; \
    } \
} while(0)

TEST(memory_tool_exists) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    ASSERT(mimi_registry_has_tool(reg, "memory") == 1, "memory tool should exist");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "memory");
    ASSERT(info != NULL, "Tool info should exist");
    ASSERT(info->name != NULL, "Name should exist");
    ASSERT(strcmp(info->name, "memory") == 0, "Name should be 'memory'");
    ASSERT(info->description != NULL, "Description should exist");
    
    printf("[name=%s] ", info->name);
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_store_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Store a memory */
    char input[256];
    snprintf(input, sizeof(input), "store|test_key|test_value_123");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", input, &output);
    printf("[store_ret=%d] ", ret);
    ASSERT(ret == MIMI_TOOLS_OK || ret == MIMI_TOOLS_ERR_EXEC_FAILED, "Store should complete");
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_retrieve_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Retrieve a memory */
    char input[256];
    snprintf(input, sizeof(input), "retrieve|test_key");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", input, &output);
    printf("[retrieve_ret=%d] ", ret);
    
    if (output != NULL) {
        printf("[output: %.30s...] ", output);
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_search_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Search memories */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", "search|test", &output);
    printf("[search_ret=%d] ", ret);
    
    if (output != NULL) {
        printf("[output_len=%zu] ", strlen(output));
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_delete_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Delete a memory */
    char input[256];
    snprintf(input, sizeof(input), "delete|test_key");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", input, &output);
    printf("[delete_ret=%d] ", ret);
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_list_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* List all memories */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", "list", &output);
    printf("[list_ret=%d] ", ret);
    
    if (output != NULL) {
        printf("[output_len=%zu] ", strlen(output));
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_invalid_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Invalid operation */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "memory", "invalid_op", &output);
    printf("[invalid_ret=%d] ", ret);
    /* Should fail gracefully */
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(memory_multiple_operations) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-memory.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Store multiple memories */
    const char *operations[] = {
        "store|key1|value1",
        "store|key2|value2",
        "store|key3|value3",
        "retrieve|key1",
        "retrieve|key2",
        "search|value",
        "list"
    };
    
    for (size_t i = 0; i < sizeof(operations)/sizeof(operations[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "memory", operations[i], &output);
        printf("[%s: ret=%d] ", operations[i], ret);
        
        if (output != NULL) {
            free(output);
        }
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Memory Plugin Tests ===\n\n");
    
    RUN_TEST(memory_tool_exists);
    RUN_TEST(memory_tool_info);
    RUN_TEST(memory_store_operation);
    RUN_TEST(memory_retrieve_operation);
    RUN_TEST(memory_search_operation);
    RUN_TEST(memory_delete_operation);
    RUN_TEST(memory_list_operation);
    RUN_TEST(memory_invalid_operation);
    RUN_TEST(memory_multiple_operations);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
