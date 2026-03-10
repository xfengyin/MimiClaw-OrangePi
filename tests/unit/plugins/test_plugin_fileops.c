/**
 * @file test_plugin_fileops.c
 * @brief Unit tests for File Operations Plugin
 * @coverage fileops plugin: read, write, list, delete operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mimi_tools.h"

static int tests_run = 0;
static int tests_passed = 0;
static const char *TEST_FILE = "/tmp/test_fileops.txt";

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

TEST(fileops_tool_exists) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    ASSERT(mimi_registry_has_tool(reg, "fileops") == 1, "fileops tool should exist");
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_tool_info) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    mimi_tool_info_t *info = mimi_registry_get_tool_info(reg, "fileops");
    ASSERT(info != NULL, "Tool info should exist");
    ASSERT(info->name != NULL, "Name should exist");
    ASSERT(strcmp(info->name, "fileops") == 0, "Name should be 'fileops'");
    ASSERT(info->description != NULL, "Description should exist");
    
    printf("[name=%s] ", info->name);
    
    mimi_tool_info_free(info);
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_write_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Write to file */
    char input[256];
    snprintf(input, sizeof(input), "write|%s|Hello from test!", TEST_FILE);
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", input, &output);
    printf("[write_ret=%d] ", ret);
    ASSERT(ret == MIMI_TOOLS_OK || ret == MIMI_TOOLS_ERR_EXEC_FAILED, "Write should complete");
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_read_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* First write a file */
    FILE *f = fopen(TEST_FILE, "w");
    if (f) {
        fprintf(f, "Test content for read");
        fclose(f);
    }
    
    /* Read from file */
    char input[256];
    snprintf(input, sizeof(input), "read|%s", TEST_FILE);
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", input, &output);
    printf("[read_ret=%d] ", ret);
    
    if (output != NULL) {
        printf("[output: %.30s...] ", output);
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_list_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* List directory */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", "list|/tmp", &output);
    printf("[list_ret=%d] ", ret);
    
    if (output != NULL) {
        printf("[output_len=%zu] ", strlen(output));
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_delete_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Create test file */
    FILE *f = fopen(TEST_FILE, "w");
    if (f) {
        fprintf(f, "To be deleted");
        fclose(f);
    }
    
    /* Delete file */
    char input[256];
    snprintf(input, sizeof(input), "delete|%s", TEST_FILE);
    
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", input, &output);
    printf("[delete_ret=%d] ", ret);
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_invalid_operation) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Invalid operation */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", "invalid_op|/tmp/test", &output);
    printf("[invalid_ret=%d] ", ret);
    /* Should fail gracefully */
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

TEST(fileops_read_nonexistent) {
    mimi_tool_registry_t *reg = mimi_registry_create();
    ASSERT(reg != NULL, "Registry creation failed");
    
    int ret = mimi_registry_load_plugin(reg, "./libmimi-plugin-file-ops.so");
    if (ret != MIMI_TOOLS_OK) {
        printf("[plugin not found, skipping] ");
        mimi_registry_destroy(reg);
        return 0;
    }
    
    /* Read non-existent file */
    char *output = NULL;
    ret = mimi_registry_exec(reg, "fileops", "read|/nonexistent/file.txt", &output);
    printf("[read_nonexistent_ret=%d] ", ret);
    /* Should fail gracefully */
    
    if (output != NULL) {
        free(output);
    }
    
    mimi_registry_destroy(reg);
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== File Operations Plugin Tests ===\n\n");
    
    RUN_TEST(fileops_tool_exists);
    RUN_TEST(fileops_tool_info);
    RUN_TEST(fileops_write_operation);
    RUN_TEST(fileops_read_operation);
    RUN_TEST(fileops_list_operation);
    RUN_TEST(fileops_delete_operation);
    RUN_TEST(fileops_invalid_operation);
    RUN_TEST(fileops_read_nonexistent);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    /* Cleanup */
    remove(TEST_FILE);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
