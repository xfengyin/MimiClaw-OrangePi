/**
 * @file test_time_plugin.c
 * @brief Time Plugin Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

TEST(time_now_format) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "now", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "timestamp") != NULL, "Missing timestamp");
    ASSERT(strstr(output, "iso") != NULL, "Missing iso");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_date_format) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "date", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "date") != NULL, "Missing date field");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_unix_format) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "unix", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "timestamp") != NULL, "Missing timestamp");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_empty_input) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with empty input");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_null_input) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", NULL, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with NULL input");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Time Plugin Tests ===\n\n");
    
    RUN_TEST(time_now_format);
    RUN_TEST(time_date_format);
    RUN_TEST(time_unix_format);
    RUN_TEST(time_empty_input);
    RUN_TEST(time_null_input);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
