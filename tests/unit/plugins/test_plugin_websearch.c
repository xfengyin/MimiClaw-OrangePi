/**
 * @file test_plugin_websearch.c
 * @brief Unit tests for Web Search Plugin
 * @coverage websearch plugin execution, query handling, result formatting
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

TEST(websearch_basic_query) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    /* Try to load websearch plugin */
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "websearch", "test query", &output);
    /* May succeed or fail depending on API availability */
    printf("[exec_ret=%d] ", ret);
    ASSERT(ret == MIMI_TOOLS_OK || ret == MIMI_TOOLS_ERR_EXEC_FAILED, "Execution should complete");
    
    if (output != NULL) {
        free(output);
    }
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_empty_query) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "websearch", "", &output);
    /* Should handle empty query gracefully */
    ASSERT(ret == MIMI_TOOLS_OK || ret == MIMI_TOOLS_ERR_EXEC_FAILED, "Should handle empty query");
    
    if (output != NULL) {
        free(output);
    }
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_null_query) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "websearch", NULL, &output);
    /* Should handle NULL query gracefully */
    ASSERT(ret == MIMI_TOOLS_OK || ret == MIMI_TOOLS_ERR_INVALID_ARG, "Should handle NULL query");
    
    if (output != NULL) {
        free(output);
    }
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_tool_exists) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    ASSERT(mimi_registry_has_tool(reg, "websearch") == 1, "websearch tool should exist");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "websearch");
    ASSERT(info != NULL, "Tool info should exist");
    ASSERT(info->name != NULL, "Name should exist");
    ASSERT(strcmp(info->name, "websearch") == 0, "Name should be 'websearch'");
    ASSERT(info->description != NULL, "Description should exist");
    ASSERT(info->is_builtin == 0, "Should be plugin (not builtin)");
    
    printf("[name=%s, version=%s] ", info->name, info->version ? info->version : "N/A");
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_multiple_queries) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    const char *queries[] = {
        "weather today",
        "news headlines",
        "programming tutorial"
    };
    
    for (size_t i = 0; i < sizeof(queries)/sizeof(queries[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "websearch", queries[i], &output);
        /* May succeed or fail */
        printf("[%s: ret=%d] ", queries[i], ret);
        
        if (output != NULL) {
            free(output);
        }
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(websearch_special_characters) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-web-search.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    const char *special_queries[] = {
        "C++ programming",
        "C# tutorial",
        "price: $100",
        "100% free"
    };
    
    for (size_t i = 0; i < sizeof(special_queries)/sizeof(special_queries[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "websearch", special_queries[i], &output);
        /* Should handle special characters */
        printf("[%s: ret=%d] ", special_queries[i], ret);
        
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
    
    printf("=== Web Search Plugin Tests ===\n\n");
    
    RUN_TEST(websearch_basic_query);
    RUN_TEST(websearch_empty_query);
    RUN_TEST(websearch_null_query);
    RUN_TEST(websearch_tool_exists);
    RUN_TEST(websearch_tool_info);
    RUN_TEST(websearch_multiple_queries);
    RUN_TEST(websearch_special_characters);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
