/**
 * @file test_plugin_echo.c
 * @brief Unit tests for Echo Plugin
 * @coverage echo plugin execution, input handling, edge cases
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

TEST(echo_simple_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", "Hello World", &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "Hello World") != NULL, "Output should contain input");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_empty_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
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
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", NULL, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with NULL");
    ASSERT(output != NULL, "Output is NULL");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_json_input) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    char *output = NULL;
    const char *input = "{\"key\": \"value\", \"number\": 42}";
    ret = mimi_registry_exec(reg, "echo", input, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strstr(output, "key") != NULL, "Output should contain 'key'");
    ASSERT(strstr(output, "value") != NULL, "Output should contain 'value'");
    
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_special_characters) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    const char *special_inputs[] = {
        "Hello\nWorld",
        "Tab\there",
        "Quote'test",
        "Double\"quote",
        "Backslash\\test",
        "Unicode: 你好世界 🌍",
        "!@#$%^&*()"
    };
    
    for (size_t i = 0; i < sizeof(special_inputs)/sizeof(special_inputs[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "echo", special_inputs[i], &output);
        ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
        ASSERT(output != NULL, "Output is NULL");
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_long_string) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    /* Create 10KB string */
    char *long_input = malloc(10240);
    ASSERT(long_input != NULL, "Failed to allocate buffer");
    memset(long_input, 'A', 10239);
    long_input[10239] = '\0';
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "echo", long_input, &output);
    ASSERT(ret == MIMI_TOOLS_OK, "Execution failed with long string");
    ASSERT(output != NULL, "Output is NULL");
    ASSERT(strlen(output) >= 1000, "Output should be long");
    
    free(long_input);
    free(output);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_multiple_executions) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    const char *inputs[] = {"first", "second", "third", "fourth", "fifth"};
    
    for (size_t i = 0; i < sizeof(inputs)/sizeof(inputs[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "echo", inputs[i], &output);
        ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
        ASSERT(output != NULL, "Output is NULL");
        ASSERT(strstr(output, inputs[i]) != NULL, "Output should match input");
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "echo");
    ASSERT(info != NULL, "Tool info is NULL");
    ASSERT(info->name != NULL, "Name is NULL");
    ASSERT(strcmp(info->name, "echo") == 0, "Name mismatch");
    ASSERT(info->description != NULL, "Description is NULL");
    ASSERT(info->is_builtin == 1, "Should be builtin");
    
    printf("[name=%s, desc=%.30s...] ", info->name, info->description);
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(echo_whitespace_handling) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK, "Failed to register echo tool");
    
    const char *whitespace_inputs[] = {
        "   leading spaces",
        "trailing spaces   ",
        "   both   ",
        "\t\ttabs\t\t",
        "\n\nnewlines\n\n"
    };
    
    for (size_t i = 0; i < sizeof(whitespace_inputs)/sizeof(whitespace_inputs[0]); i++) {
        char *output = NULL;
        ret = mimi_registry_exec(reg, "echo", whitespace_inputs[i], &output);
        ASSERT(ret == MIMI_TOOLS_OK, "Execution failed");
        ASSERT(output != NULL, "Output is NULL");
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Echo Plugin Tests ===\n\n");
    
    RUN_TEST(echo_simple_string);
    RUN_TEST(echo_empty_string);
    RUN_TEST(echo_null_input);
    RUN_TEST(echo_json_input);
    RUN_TEST(echo_special_characters);
    RUN_TEST(echo_long_string);
    RUN_TEST(echo_multiple_executions);
    RUN_TEST(echo_tool_info);
    RUN_TEST(echo_whitespace_handling);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
