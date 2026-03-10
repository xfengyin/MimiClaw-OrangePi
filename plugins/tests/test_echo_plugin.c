/**
 * @file test_echo_plugin.c
 * @brief Echo Plugin Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

TEST(echo_simple_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", "Hello World", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "Hello World") != NULL, "Output should contain 'Hello World'");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_json_input) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    const char *input = "{\"key\":\"value\"}";
    ret = mimi_registry_exec(reg, "echo", input, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "key") != NULL, "Output should contain 'key'");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_empty_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", "", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with empty string");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_null_input) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", NULL, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with NULL");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "null") != NULL, "Output should contain 'null'");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_special_characters) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *output = NULL;
    const char *input = "Hello\nWorld\t\"Test\"";
    ret = mimi_registry_exec(reg, "echo", input, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_long_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to load plugin");
    
    char *long_input = malloc(10000);
    ASSERT(long_input != NULL, "Failed to allocate test buffer");
    memset(long_input, 'A', 9999);
    long_input[9999] = '\0';
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", long_input, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with long string");
    ASSERT(output != NULL, "Output is NULL");
    
    free(long_input);
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Echo Plugin Tests ===\n\n");
    
    RUN_TEST(echo_simple_string);
    RUN_TEST(echo_json_input);
    RUN_TEST(echo_empty_string);
    RUN_TEST(echo_null_input);
    RUN_TEST(echo_special_characters);
    RUN_TEST(echo_long_string);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
