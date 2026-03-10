/**
 * @file example_tools.c
 * @brief Example: Using libmimi-tools
 */

#include <stdio.h>
#include <stdlib.h>
#include "mimi_tools.h"

int main(void)
{
    printf("=== MimiTools Example ===\n\n");
    printf("Library version: %s\n\n", mimi_tools_version());
    
    /* Create registry */
    mimi_tool_registry_t *reg = mimi_registry_create();
    if (reg == NULL) {
        fprintf(stderr, "Registry create failed\n");
        return 1;
    }
    printf("✓ Registry created\n");
    
    /* Register built-in tools */
    int ret = mimi_tools_register_time(reg);
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Tool 'time' registered\n");
    }
    
    ret = mimi_tools_register_echo(reg);
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Tool 'echo' registered\n");
    }
    
    /* Check tool exists */
    if (mimi_registry_has_tool(reg, "time")) {
        printf("✓ Tool 'time' exists\n");
    }
    
    /* Execute time tool */
    char *output;
    ret = mimi_registry_exec(reg, "time", "", &output);
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Time tool output: %s\n", output);
        free(output);
    }
    
    /* Execute echo tool */
    ret = mimi_registry_exec(reg, "echo", "Hello, World!", &output);
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Echo tool output: %s\n", output);
        free(output);
    }
    
    /* Get tool info */
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "echo");
    if (info != NULL) {
        printf("✓ Tool info:\n");
        printf("    Name: %s\n", info->name);
        printf("    Description: %s\n", info->description);
        printf("    Built-in: %s\n", info->is_builtin ? "yes" : "no");
        mimi_tool_info_free(info);
    }
    
    /* List all tools */
    mimi_tool_info_t **tools;
    int count;
    ret = mimi_registry_list(reg, &tools, &count);
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Registered tools (%d):\n", count);
        for (int i = 0; i < count; i++) {
            printf("  %d. %s - %s\n", i + 1, 
                   tools[i]->name, tools[i]->description);
            mimi_tool_info_free(tools[i]);
        }
        free(tools);
    }
    
    /* Unregister tool */
    ret = mimi_registry_unregister_tool(reg, "echo");
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Tool 'echo' unregistered\n");
    }
    
    /* Cleanup */
    mimi_registry_destroy(reg);
    printf("✓ Registry destroyed\n");
    
    printf("\n=== Example Complete ===\n");
    return 0;
}
