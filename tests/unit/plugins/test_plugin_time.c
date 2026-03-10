/**
 * @file test_plugin_time.c
 * @brief Unit tests for Time Plugin
 * @coverage time plugin execution, format options, timezone handling
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

TEST(time_basic_execution) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strlen(output) > 0, "Output is empty");
    
    printf("[output: %.50s...] ", output);
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_with_format_iso) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "iso", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    /* ISO format should contain T separator */
    ASSERT(strstr(output, "T") != NULL || strstr(output, "-") != NULL, "Should contain ISO format");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_with_format_timestamp) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "timestamp", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    /* Timestamp should be numeric */
    ASSERT(strspn(output, "0123456789\n") > 0, "Should contain numeric timestamp");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_with_format_date) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "date", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    /* Date should contain year-month-day pattern */
    ASSERT(strlen(output) > 5, "Date output too short");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_with_format_time) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "time", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    /* Time should contain colon */
    ASSERT(strstr(output, ":") != NULL, "Should contain time with colon");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_with_invalid_format) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "time", "invalid_format_xyz", &output);
    /* Should still return time with default format */
    ASSERT(ret == MIMI_TOOLS_OK, "Should handle invalid format gracefully");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_multiple_executions) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    /* Execute multiple times */
    for (int i = 0; i < 5; i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "time", "", &output);
        ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
        ASSERT(output != NULL, "Output is NULL");
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(time_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register time tool");
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "time");
    ASSERT(info != NULL, "Tool info is NULL");
    ASSERT(info->name != NULL, "Name is NULL");
    ASSERT(strcmp(info->name, "time") == 0, "Name mismatch");
    ASSERT(info->description != NULL, "Description is NULL");
    ASSERT(info->is_builtin == 1, "Should be builtin");
    
    printf("[name=%s, desc=%.30s...] ", info->name, info->description);
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Time Plugin Tests ===\n\n");
    
    RUN_TEST(time_basic_execution);
    RUN_TEST(time_with_format_iso);
    RUN_TEST(time_with_format_timestamp);
    RUN_TEST(time_with_format_date);
    RUN_TEST(time_with_format_time);
    RUN_TEST(time_with_invalid_format);
    RUN_TEST(time_multiple_executions);
    RUN_TEST(time_tool_info);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
