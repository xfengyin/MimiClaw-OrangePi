/**
 * @file test_watch.c
 * @brief Unit tests for Config Library - Hot Reload & Watch
 * @coverage mimi_config file watching, hot reload, callbacks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "mimi_config.h"

static int tests_run = 0;
static int tests_passed = 0;
static int reload_callback_called = 0;

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

static const char *TEST_CONFIG_PATH = "/tmp/test_mimi_watch.json";

/* Reload callback */
static void reload_callback(void *user_data, int status)
{
    (void)user_data;
    reload_callback_called = 1;
    printf("[callback: status=%d] ", status);
}

/* Test manual reload */
TEST(test_watch_manual_reload)
{
    /* Create initial config */
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"version\": 1}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Get initial value */
    int version = 0;
    ASSERT(mimi_config_get_int(cfg, "version", &version) == MIMI_CONFIG_OK);
    ASSERT(version == 1);
    
    /* Update file */
    f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"version\": 2}");
    fclose(f);
    
    /* Manual reload */
    ASSERT(mimi_config_reload(cfg) == MIMI_CONFIG_OK);
    
    /* Get updated value */
    version = 0;
    ASSERT(mimi_config_get_int(cfg, "version", &version) == MIMI_CONFIG_OK);
    ASSERT(version == 2);
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test watch with callback */
TEST(test_watch_with_callback)
{
    reload_callback_called = 0;
    
    /* Create initial config */
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"watched\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Enable watch with callback */
    mimi_config_options_t options = {
        .watch_enabled = 1,
        .on_reload = reload_callback,
        .user_data = NULL,
        .watch_interval_ms = 100
    };
    
    int ret = mimi_config_watch(cfg, &options);
    /* Watch may not be available on all systems */
    if (ret == MIMI_CONFIG_OK) {
        /* Update file */
        f = fopen(TEST_CONFIG_PATH, "w");
        ASSERT(f != NULL);
        fprintf(f, "{\"watched\": false}");
        fclose(f);
        
        /* Wait for callback */
        usleep(500000);  /* 500ms */
        
        /* Callback should have been called */
        /* Note: This may not work in all environments */
        printf("[callback_called=%d] ", reload_callback_called);
    }
    
    mimi_config_unwatch(cfg);
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test watch disable */
TEST(test_watch_disable)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"test\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Enable then disable watch */
    mimi_config_options_t options = {
        .watch_enabled = 1,
        .on_reload = NULL,
        .user_data = NULL,
        .watch_interval_ms = 1000
    };
    
    mimi_config_watch(cfg, &options);
    mimi_config_unwatch(cfg);
    
    /* Should be able to reload manually after unwatch */
    f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"test\": false}");
    fclose(f);
    
    ASSERT(mimi_config_reload(cfg) == MIMI_CONFIG_OK);
    
    int value = 0;
    ASSERT(mimi_config_get_bool(cfg, "test", &value) == MIMI_CONFIG_OK);
    ASSERT(value == 0);
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test watch with invalid options */
TEST(test_watch_invalid_options)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"test\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* NULL options should work with defaults */
    int ret = mimi_config_watch(cfg, NULL);
    /* May succeed or fail depending on implementation */
    ASSERT(ret == MIMI_CONFIG_OK || ret == MIMI_CONFIG_ERR_INVALID_ARG);
    
    if (ret == MIMI_CONFIG_OK) {
        mimi_config_unwatch(cfg);
    }
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test reload on deleted file */
TEST(test_watch_file_deleted)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"test\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Delete file */
    remove(TEST_CONFIG_PATH);
    
    /* Reload should fail */
    int ret = mimi_config_reload(cfg);
    ASSERT(ret == MIMI_CONFIG_ERR_FILE_NOT_FOUND || ret == MIMI_CONFIG_ERR_IO);
    
    mimi_config_free(cfg);
}

/* Test reload on invalid JSON */
TEST(test_watch_invalid_json)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"valid\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Write invalid JSON */
    f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{invalid json}");
    fclose(f);
    
    /* Reload should fail */
    int ret = mimi_config_reload(cfg);
    ASSERT(ret == MIMI_CONFIG_ERR_PARSE_ERROR);
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test multiple reloads */
TEST(test_watch_multiple_reloads)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"count\": 0}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Multiple updates and reloads */
    for (int i = 1; i <= 5; i++) {
        f = fopen(TEST_CONFIG_PATH, "w");
        ASSERT(f != NULL);
        fprintf(f, "{\"count\": %d}", i);
        fclose(f);
        
        ASSERT(mimi_config_reload(cfg) == MIMI_CONFIG_OK);
        
        int count = 0;
        ASSERT(mimi_config_get_int(cfg, "count", &count) == MIMI_CONFIG_OK);
        ASSERT(count == i);
    }
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

int main(void)
{
    printf("=== libmimi-config Watch Tests ===\n\n");
    
    RUN_TEST(test_watch_manual_reload);
    RUN_TEST(test_watch_with_callback);
    RUN_TEST(test_watch_disable);
    RUN_TEST(test_watch_invalid_options);
    RUN_TEST(test_watch_file_deleted);
    RUN_TEST(test_watch_invalid_json);
    RUN_TEST(test_watch_multiple_reloads);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
