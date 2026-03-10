/**
 * @file test_registry.c
 * @brief Unit tests for Tools Library - Registry Operations
 * @coverage mimi_tools registry create/destroy, tool registration, listing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mimi_tools.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("✓\n"); \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { \
    printf("✗ FAILED: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
} } while(0)

/* Test registry create and destroy */
TEST(test_registry_create_destroy)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    mimi_registry_destroy(reg);
}

/* Test registry destroy with NULL */
TEST(test_registry_destroy_null)
{
    mimi_registry_destroy(NULL);
}

/* Test register built-in time tool */
TEST(test_registry_register_time)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_tools_register_time(reg);
    ASSERT(ret == MIMI_TOOLS_OK);
    
    /* Verify tool exists */
    ASSERT(mimi_registry_has_tool(reg, "time") == 1);
    
    mimi_registry_destroy(reg);
}

/* Test register built-in echo tool */
TEST(test_registry_register_echo)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_tools_register_echo(reg);
    ASSERT(ret == MIMI_TOOLS_OK);
    
    /* Verify tool exists */
    ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
    
    mimi_registry_destroy(reg);
}

/* Test register multiple tools */
TEST(test_registry_register_multiple)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_tools_register_time(reg) == MIMI_TOOLS_OK);
    ASSERT(mimi_tools_register_echo(reg) == MIMI_TOOLS_OK);
    
    ASSERT(mimi_registry_has_tool(reg, "time") == 1);
    ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
    
    mimi_registry_destroy(reg);
}

/* Test unregister tool */
TEST(test_registry_unregister_tool)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_tools_register_echo(reg) == MIMI_TOOLS_OK);
    ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
    
    int ret = mimi_registry_unregister_tool(reg, "echo");
    ASSERT(ret == MIMI_TOOLS_OK);
    ASSERT(mimi_registry_has_tool(reg, "echo") == 0);
    
    /* Unregister non-existent */
    ret = mimi_registry_unregister_tool(reg, "nonexistent");
    ASSERT(ret == MIMI_TOOLS_ERR_NOT_FOUND);
    
    mimi_registry_destroy(reg);
}

/* Test tool list */
TEST(test_registry_list)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    /* Empty registry */
    mimi_tool_info_t **tools = NULL;
    int count = 0;
    ASSERT(mimi_registry_list(reg, &tools, &count) == MIMI_TOOLS_OK);
    ASSERT(count == 0);
    
    /* Register tools */
    ASSERT(mimi_tools_register_time(reg) == MIMI_TOOLS_OK);
    ASSERT(mimi_tools_register_echo(reg) == MIMI_TOOLS_OK);
    
    /* List tools */
    tools = NULL;
    count = 0;
    ASSERT(mimi_registry_list(reg, &tools, &count) == MIMI_TOOLS_OK);
    ASSERT(count == 2);
    
    /* Verify tool info */
    for (int i = 0; i < count; i++) {
        ASSERT(tools[i]->name != NULL);
        ASSERT(tools[i]->description != NULL);
        mimi_tool_info_free(tools[i]);
    }
    free(tools);
    
    mimi_registry_destroy(reg);
}

/* Test get tool info */
TEST(test_registry_get_tool_info)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_tools_register_time(reg) == MIMI_TOOLS_OK);
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "time");
    ASSERT(info != NULL);
    ASSERT(info->name != NULL);
    ASSERT(strcmp(info->name, "time") == 0);
    ASSERT(info->description != NULL);
    ASSERT(info->is_builtin == 1);
    
    mimi_tool_info_free(info);
    
    /* Non-existent tool */
    info = mimi_registry_get_tool_info(reg, "nonexistent");
    ASSERT(info == NULL);
    
    mimi_registry_destroy(reg);
}

/* Test has_tool with various names */
TEST(test_registry_has_tool)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_registry_has_tool(reg, "nonexistent") == 0);
    
    ASSERT(mimi_tools_register_echo(reg) == MIMI_TOOLS_OK);
    ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
    ASSERT(mimi_registry_has_tool(reg, "Echo") == 0);  /* Case sensitive */
    ASSERT(mimi_registry_has_tool(reg, "") == 0);
    
    mimi_registry_destroy(reg);
}

/* Test version and error functions */
TEST(test_registry_utility_functions)
{
    ASSERT(mimi_tools_version() != NULL);
    ASSERT(mimi_tools_strerror(MIMI_TOOLS_OK) != NULL);
    ASSERT(mimi_tools_strerror(MIMI_TOOLS_ERR_INVALID_ARG) != NULL);
    ASSERT(mimi_tools_strerror(-999) != NULL);
}

/* Test registry with NULL operations */
TEST(test_registry_null_operations)
{
    /* NULL registry operations */
    ASSERT(mimi_registry_has_tool(NULL, "test") == 0);
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(NULL, "test");
    ASSERT(info == NULL);
    
    mimi_tool_info_t **tools = NULL;
    int count = 0;
    int ret = mimi_registry_list(NULL, &tools, &count);
    ASSERT(ret != MIMI_TOOLS_OK);
    
    /* NULL tool name */
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    ASSERT(mimi_registry_has_tool(reg, NULL) == 0);
    
    mimi_registry_destroy(reg);
}

/* Test tool execution with built-in echo */
TEST(test_registry_exec_echo)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_tools_register_echo(reg) == MIMI_TOOLS_OK);
    
    char *output = NULL;
    int ret = mimi_registry_exec(reg, "echo", "Hello World", &output);
    ASSERT(ret == MIMI_TOOLS_OK);
    ASSERT(output != NULL);
    ASSERT(strstr(output, "Hello World") != NULL);
    
    free(output);
    mimi_registry_destroy(reg);
}

/* Test tool execution with built-in time */
TEST(test_registry_exec_time)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    ASSERT(mimi_tools_register_time(reg) == MIMI_TOOLS_OK);
    
    char *output = NULL;
    int ret = mimi_registry_exec(reg, "time", "", &output);
    ASSERT(ret == MIMI_TOOLS_OK);
    ASSERT(output != NULL);
    /* Output should contain some date/time info */
    ASSERT(strlen(output) > 0);
    
    free(output);
    mimi_registry_destroy(reg);
}

/* Test execution with non-existent tool */
TEST(test_registry_exec_nonexistent)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    char *output = NULL;
    int ret = mimi_registry_exec(reg, "nonexistent", "input", &output);
    ASSERT(ret == MIMI_TOOLS_ERR_NOT_FOUND);
    ASSERT(output == NULL);
    
    mimi_registry_destroy(reg);
}

int main(void)
{
    printf("=== libmimi-tools Registry Tests ===\n\n");
    
    RUN_TEST(test_registry_create_destroy);
    RUN_TEST(test_registry_destroy_null);
    RUN_TEST(test_registry_register_time);
    RUN_TEST(test_registry_register_echo);
    RUN_TEST(test_registry_register_multiple);
    RUN_TEST(test_registry_unregister_tool);
    RUN_TEST(test_registry_list);
    RUN_TEST(test_registry_get_tool_info);
    RUN_TEST(test_registry_has_tool);
    RUN_TEST(test_registry_utility_functions);
    RUN_TEST(test_registry_null_operations);
    RUN_TEST(test_registry_exec_echo);
    RUN_TEST(test_registry_exec_time);
    RUN_TEST(test_registry_exec_nonexistent);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
