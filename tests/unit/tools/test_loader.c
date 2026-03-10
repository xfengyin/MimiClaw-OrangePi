/**
 * @file test_loader.c
 * @brief Unit tests for Tools Library - Plugin Loading
 * @coverage mimi_tools plugin load/unload, dynamic libraries
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

/* Test load plugin from .so file */
TEST(test_loader_load_plugin)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    /* Try to load echo plugin */
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    /* May succeed or fail depending on file location */
    printf("[load_ret=%d] ", ret);
    
    if (ret == MIMI_TOOLS_OK) {
        ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
        
        /* Test execution */
        char *output = NULL;
        ret = mimi_registry_exec(reg, "echo", "test", &output);
        ASSERT(ret == MIMI_TOOLS_OK);
        ASSERT(output != NULL);
        free(output);
    }
    
    mimi_registry_destroy(reg);
}

/* Test load non-existent plugin */
TEST(test_loader_load_nonexistent)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_load_plugin(reg, "./nonexistent_plugin.so");
    ASSERT(ret == MIMI_TOOLS_ERR_LOAD_FAILED || ret == MIMI_TOOLS_ERR_NOT_FOUND);
    
    mimi_registry_destroy(reg);
}

/* Test load plugin with NULL path */
TEST(test_loader_load_null_path)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_load_plugin(reg, NULL);
    ASSERT(ret == MIMI_TOOLS_ERR_INVALID_ARG);
    
    mimi_registry_destroy(reg);
}

/* Test unload plugin */
TEST(test_loader_unload_plugin)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    
    if (ret == MIMI_TOOLS_OK) {
        ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
        
        ret = mimi_registry_unload_plugin(reg, "echo");
        ASSERT(ret == MIMI_TOOLS_OK);
        ASSERT(mimi_registry_has_tool(reg, "echo") == 0);
    }
    
    mimi_registry_destroy(reg);
}

/* Test unload non-existent plugin */
TEST(test_loader_unload_nonexistent)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_unload_plugin(reg, "nonexistent");
    ASSERT(ret == MIMI_TOOLS_ERR_NOT_FOUND || ret == MIMI_TOOLS_ERR_UNLOAD_FAILED);
    
    mimi_registry_destroy(reg);
}

/* Test get plugin */
TEST(test_loader_get_plugin)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    /* Get non-existent plugin */
    const mimi_plugin_t *plugin = mimi_registry_get_plugin(reg, "nonexistent");
    ASSERT(plugin == NULL);
    
    /* Load and get */
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    if (ret == MIMI_TOOLS_OK) {
        plugin = mimi_registry_get_plugin(reg, "echo");
        ASSERT(plugin != NULL);
        ASSERT(plugin->meta.name != NULL);
    }
    
    mimi_registry_destroy(reg);
}

/* Test load plugins from directory */
TEST(test_loader_load_plugins_dir)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    /* Try to load from build directory */
    int count = mimi_registry_load_plugins_dir(reg, "./");
    printf("[loaded=%d] ", count);
    /* May load 0 or more depending on files present */
    ASSERT(count >= 0);
    
    mimi_registry_destroy(reg);
}

/* Test load plugins from non-existent directory */
TEST(test_loader_load_dir_nonexistent)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int count = mimi_registry_load_plugins_dir(reg, "/nonexistent/dir");
    ASSERT(count < 0);
    
    mimi_registry_destroy(reg);
}

/* Test load plugins with NULL directory */
TEST(test_loader_load_dir_null)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int count = mimi_registry_load_plugins_dir(reg, NULL);
    ASSERT(count == MIMI_TOOLS_ERR_INVALID_ARG || count < 0);
    
    mimi_registry_destroy(reg);
}

/* Test multiple plugin loads */
TEST(test_loader_multiple_plugins)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    /* Load echo plugin */
    int ret1 = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    printf("[echo_ret=%d] ", ret1);
    
    /* Load time plugin (if exists as .so) */
    int ret2 = mimi_registry_load_plugin(reg, "./libmimi-plugin-time.so");
    printf("[time_ret=%d] ", ret2);
    
    /* Verify loaded tools */
    if (ret1 == MIMI_TOOLS_OK) {
        ASSERT(mimi_registry_has_tool(reg, "echo") == 1);
    }
    if (ret2 == MIMI_TOOLS_OK) {
        ASSERT(mimi_registry_has_tool(reg, "time") == 1);
    }
    
    mimi_registry_destroy(reg);
}

/* Test plugin execution after unload */
TEST(test_loader_exec_after_unload)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    
    if (ret == MIMI_TOOLS_OK) {
        /* Execute before unload */
        char *output = NULL;
        ret = mimi_registry_exec(reg, "echo", "before", &output);
        ASSERT(ret == MIMI_TOOLS_OK);
        free(output);
        
        /* Unload */
        mimi_registry_unload_plugin(reg, "echo");
        
        /* Execute after unload - should fail */
        output = NULL;
        ret = mimi_registry_exec(reg, "echo", "after", &output);
        ASSERT(ret == MIMI_TOOLS_ERR_NOT_FOUND);
    }
    
    mimi_registry_destroy(reg);
}

/* Test plugin metadata */
TEST(test_loader_plugin_metadata)
{
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL);
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-echo.so");
    
    if (ret == MIMI_TOOLS_OK) {
        mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "echo");
        ASSERT(info != NULL);
        
        /* Check metadata */
        ASSERT(info->name != NULL);
        ASSERT(info->version != NULL);
        ASSERT(info->description != NULL);
        ASSERT(info->is_builtin == 0);  /* Loaded from plugin */
        ASSERT(info->plugin_path != NULL);
        
        printf("[name=%s, version=%s] ", info->name, info->version);
        
        mimi_tool_info_free(info);
    }
    
    mimi_registry_destroy(reg);
}

int main(void)
{
    printf("=== libmimi-tools Loader Tests ===\n\n");
    
    RUN_TEST(test_loader_load_plugin);
    RUN_TEST(test_loader_load_nonexistent);
    RUN_TEST(test_loader_load_null_path);
    RUN_TEST(test_loader_unload_plugin);
    RUN_TEST(test_loader_unload_nonexistent);
    RUN_TEST(test_loader_get_plugin);
    RUN_TEST(test_loader_load_plugins_dir);
    RUN_TEST(test_loader_load_dir_nonexistent);
    RUN_TEST(test_loader_load_dir_null);
    RUN_TEST(test_loader_multiple_plugins);
    RUN_TEST(test_loader_exec_after_unload);
    RUN_TEST(test_loader_plugin_metadata);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
