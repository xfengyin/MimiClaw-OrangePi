/**
 * @file test_all_plugins.c
 * @brief Simple test program for all plugins
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "mimi_tools.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== MimiClaw Plugin System Test ===\n\n");
    
    /* Create registry */
    printf("Creating registry...\n");
    mimi_tool_registry_t *reg = mimi_registry_create();
    if (reg == NULL) {
        fprintf(stderr, "Failed to create registry\n");
        return 1;
    }
    printf("Registry created at %p\n", (void*)reg);
    
    /* Test loading time plugin */
    printf("\nLoading time plugin...\n");
    int ret = mimi_registry_load_plugin(reg, "./build/libmimi-plugin-time.so");
    printf("Load result: %d (%s)\n", ret, mimi_tools_strerror(ret));
    
    if (ret != MIMI_TOOLS_OK) {
        /* Try with absolute path */
        char abs_path[512];
        snprintf(abs_path, sizeof(abs_path), "%s/build/libmimi-plugin-time.so", getenv("PWD"));
        printf("Trying absolute path: %s\n", abs_path);
        ret = mimi_registry_load_plugin(reg, abs_path);
        printf("Load result: %d (%s)\n", ret, mimi_tools_strerror(ret));
    }
    
    if (ret == MIMI_TOOLS_OK) {
        printf("✓ Time plugin loaded\n");
        
        /* Test execution */
        printf("\nExecuting time plugin...\n");
        char *output = NULL;
        ret = mimi_registry_exec(reg, "time", "now", &output);
        printf("Exec result: %d\n", ret);
        if (ret == MIMI_TOOLS_OK && output != NULL) {
            printf("Output: %s\n", output);
            free(output);
        }
    }
    
    /* Cleanup */
    printf("\nDestroying registry...\n");
    mimi_registry_destroy(reg);
    printf("✓ Done\n");
    
    return 0;
}
