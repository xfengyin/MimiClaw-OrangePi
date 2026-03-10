/**
 * @file test_plugin_loader.c
 * @brief Plugin Loader Tests
 * 
 * Tests for dynamic plugin loading mechanism.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "mimi_tools.h"

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

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

/* ============================================================================
 * Tests
 * ============================================================================ */

TEST(registry_create_destroy) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry should not be NULL");
    mimi_registry_destroy(reg);
    return 0;
}

TEST(load_time_plugin) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load time plugin");
    
    ASSERT(mimi_registry_has_tool(reg, "time") == 1, "Time tool not found");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(load_echo_plugin) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load echo plugin");
    
    ASSERT(mimi_registry_has_tool(reg, "echo") == 1, "Echo tool not found");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_plugin_exec) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load time plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "now", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Time execution failed");
    ASSERT(output != NULL, "Output should not be NULL");
    ASSERT(strstr(output, "timestamp") != NULL, "Output should contain timestamp");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_plugin_exec) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load echo plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", "Hello World", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Echo execution failed");
    ASSERT(output != NULL, "Output should not be NULL");
    ASSERT(strstr(output, "Hello World") != NULL, "Output should contain echoed text");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(unload_plugin) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load echo plugin");
    
    ret = mimi_registry_unload_plugin(reg, "echo");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to unload plugin");
    
    ASSERT(mimi_registry_has_tool(reg, "echo") == 0, "Echo tool should be unloaded");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(load_nonexistent_plugin) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./nonexistent.so");
    ASSERT(ret != MIMI_TOOLS_OK, "Should fail to load nonexistent plugin");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(get_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load time plugin");
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "time");
    ASSERT(info != NULL, "Tool info should not be NULL");
    ASSERT(info->name != NULL, "Name should not be NULL");
    ASSERT(strcmp(info->name, "time") == 0, "Name should be 'time'");
    ASSERT(info->is_builtin == 0, "Should not be builtin");
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(list_tools) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    
    mimi_tool_info_t **tools = NULL;
    int count = 0;
    int ret = mimi_registry_list(reg, &tools, &count);
    ASSERT(ret == MIMI_TOOLS_OK, "List failed");
    ASSERT(count == 2, "Should have 2 tools");
    
    for (int i = 0; i < count; i++) {
        mimi_tool_info_free(tools[i]);
    }
    free(tools);
    
    mimi_registry_destroy(reg);
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== MimiClaw Plugin Loader Tests ===\n\n");
    
    printf("Registry Tests:\n");
    RUN_TEST(registry_create_destroy);
    
    printf("\nPlugin Loading Tests:\n");
    RUN_TEST(load_time_plugin);
    RUN_TEST(load_echo_plugin);
    RUN_TEST(load_nonexistent_plugin);
    
    printf("\nPlugin Execution Tests:\n");
    RUN_TEST(time_plugin_exec);
    RUN_TEST(echo_plugin_exec);
    
    printf("\nPlugin Management Tests:\n");
    RUN_TEST(unload_plugin);
    RUN_TEST(get_tool_info);
    RUN_TEST(list_tools);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
