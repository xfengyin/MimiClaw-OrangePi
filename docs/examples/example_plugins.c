/**
 * @file example_plugins.c
 * @brief Plugin system example
 * 
 * Demonstrates:
 * - Tool registry creation
 * - Built-in tool registration
 * - Plugin loading from .so files
 * - Tool execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mimi_tools.h"

int main(void)
{
    printf("=== MimiClaw Plugin System Example ===\n\n");
    
    /* Create tool registry */
    mimi_tool_registry_t *reg = mimi_registry_create();
    if (!reg) {
        fprintf(stderr, "Failed to create registry\n");
        return 1;
    }
    
    printf("Registry created\n\n");
    
    /* Register built-in tools */
    printf("--- Registering Built-in Tools ---\n");
    
    if (mimi_tools_register_time(reg) == MIMI_TOOLS_OK) {
        printf("✓ Time tool registered\n");
    }
    
    if (mimi_tools_register_echo(reg) == MIMI_TOOLS_OK) {
        printf("✓ Echo tool registered\n");
    }
    
    /* List available tools */
    printf("\n--- Available Tools ---\n");
    mimi_tool_info_t **tools = NULL;
    int count = 0;
    
    if (mimi_registry_list(reg, &tools, &count) == MIMI_TOOLS_OK) {
        printf("Total tools: %d\n", count);
        for (int i = 0; i < count; i++) {
            printf("  %d. %s (v%s) - %s\n", 
                   i + 1,
                   tools[i]->name,
                   tools[i]->version,
                   tools[i]->description);
            mimi_tool_info_free(tools[i]);
        }
        free(tools);
    }
    
    /* Execute echo tool */
    printf("\n--- Executing Echo Tool ---\n");
    char *output = NULL;
    
    if (mimi_registry_exec(reg, "echo", "Hello from MimiClaw!", &output) == MIMI_TOOLS_OK) {
        printf("Echo output: %s\n", output);
        free(output);
    }
    
    /* Execute time tool with different formats */
    printf("\n--- Executing Time Tool ---\n");
    
    const char *formats[] = {"", "iso", "timestamp", "date", "time"};
    for (size_t i = 0; i < sizeof(formats)/sizeof(formats[0]); i++) {
        output = NULL;
        if (mimi_registry_exec(reg, "time", formats[i], &output) == MIMI_TOOLS_OK) {
            printf("Time (%s): %s\n", 
                   formats[i][0] ? formats[i] : "default", 
                   output);
            free(output);
        }
    }
    
    /* Load plugin from .so file */
    printf("\n--- Loading External Plugins ---\n");
    
    const char *plugins[] = {
        "./libmimi-plugin-echo.so",
        "./libmimi-plugin-time.so",
        "./libmimi-plugin-web-search.so",
        "./libmimi-plugin-file-ops.so",
        "./libmimi-plugin-memory.so"
    };
    
    for (size_t i = 0; i < sizeof(plugins)/sizeof(plugins[0]); i++) {
        int ret = mimi_registry_load_plugin(reg, plugins[i]);
        if (ret == MIMI_TOOLS_OK) {
            printf("✓ Loaded: %s\n", plugins[i]);
        } else {
            printf("⚠ Could not load: %s (%s)\n", 
                   plugins[i], mimi_tools_strerror(ret));
        }
    }
    
    /* Get tool info */
    printf("\n--- Tool Information ---\n");
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "echo");
    if (info) {
        printf("Tool: %s\n", info->name);
        printf("Version: %s\n", info->version);
        printf("Description: %s\n", info->description);
        printf("Author: %s\n", info->author ? info->author : "Unknown");
        printf("Type: %s\n", info->is_builtin ? "Built-in" : "Plugin");
        if (info->plugin_path) {
            printf("Path: %s\n", info->plugin_path);
        }
        mimi_tool_info_free(info);
    }
    
    /* Check if tool exists */
    printf("\n--- Tool Existence Check ---\n");
    printf("Has 'echo': %s\n", mimi_registry_has_tool(reg, "echo") ? "Yes" : "No");
    printf("Has 'time': %s\n", mimi_registry_has_tool(reg, "time") ? "Yes" : "No");
    printf("Has 'nonexistent': %s\n", mimi_registry_has_tool(reg, "nonexistent") ? "Yes" : "No");
    
    /* Unregister a tool */
    printf("\n--- Unregistering Echo Tool ---\n");
    if (mimi_registry_unregister_tool(reg, "echo") == MIMI_TOOLS_OK) {
        printf("✓ Echo tool unregistered\n");
        printf("Has 'echo' now: %s\n", mimi_registry_has_tool(reg, "echo") ? "Yes" : "No");
    }
    
    /* Re-register for cleanup demo */
    mimi_tools_register_echo(reg);
    
    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    mimi_registry_destroy(reg);
    printf("Registry destroyed\n");
    
    printf("\nDone!\n");
    
    return 0;
}

/*
 * Compile:
 * gcc -I../../libs/libmimi-tools/include \
 *     example_plugins.c \
 *     -L../../libs/build/lib \
 *     -lmimi-tools -ldl \
 *     -o example_plugins
 *
 * Run (from plugins/build directory):
 * cd plugins/build
 * ../../docs/examples/example_plugins
 */
