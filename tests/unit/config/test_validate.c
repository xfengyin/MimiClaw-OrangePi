/**
 * @file test_validate.c
 * @brief Unit tests for Config Library - Validation
 * @coverage mimi_config validation, required keys, error messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mimi_config.h"

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

/* Test validation with all required keys present */
TEST(test_validate_all_keys_present)
{
    const char *json = "{\"host\": \"localhost\", \"port\": 8080, \"enabled\": true}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"host", "port", "enabled"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    ASSERT(ret == MIMI_CONFIG_OK);
    ASSERT(error_msg == NULL);
    
    mimi_config_free(cfg);
}

/* Test validation with missing keys */
TEST(test_validate_missing_keys)
{
    const char *json = "{\"host\": \"localhost\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"host", "port", "enabled"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    ASSERT(ret == MIMI_CONFIG_ERR_VALIDATION);
    ASSERT(error_msg != NULL);
    printf("[error: %s] ", error_msg);
    
    free(error_msg);
    mimi_config_free(cfg);
}

/* Test validation with empty required list */
TEST(test_validate_empty_required)
{
    const char *json = "{\"any\": \"value\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    char *error_msg = NULL;
    int ret = mimi_config_validate(cfg, NULL, 0, &error_msg);
    ASSERT(ret == MIMI_CONFIG_OK);
    
    mimi_config_free(cfg);
}

/* Test validation with nested keys */
TEST(test_validate_nested_keys)
{
    const char *json = "{"
        "\"database\": {"
            "\"host\": \"localhost\","
            "\"port\": 5432"
        "},"
        "\"cache\": {"
            "\"enabled\": true"
        "}"
    "}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"database.host", "database.port", "cache.enabled"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    ASSERT(ret == MIMI_CONFIG_OK);
    
    mimi_config_free(cfg);
}

/* Test validation with some nested keys missing */
TEST(test_validate_nested_missing)
{
    const char *json = "{"
        "\"database\": {"
            "\"host\": \"localhost\""
        "}"
    "}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"database.host", "database.port", "database.ssl"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    ASSERT(ret == MIMI_CONFIG_ERR_VALIDATION);
    ASSERT(error_msg != NULL);
    
    free(error_msg);
    mimi_config_free(cfg);
}

/* Test validation with NULL config */
TEST(test_validate_null_config)
{
    const char *required[] = {"key1", "key2"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(NULL, required, 2, &error_msg);
    ASSERT(ret == MIMI_CONFIG_ERR_INVALID_ARG || ret == MIMI_CONFIG_ERR_VALIDATION);
    
    if (error_msg != NULL) {
        free(error_msg);
    }
}

/* Test validation with duplicate required keys */
TEST(test_validate_duplicate_keys)
{
    const char *json = "{\"key1\": \"value1\", \"key2\": \"value2\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"key1", "key1", "key2"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    /* Should pass since key1 exists (duplicates don't matter) */
    ASSERT(ret == MIMI_CONFIG_OK);
    
    mimi_config_free(cfg);
}

/* Test validation error message content */
TEST(test_validate_error_message_content)
{
    const char *json = "{\"existing\": \"value\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"missing_key"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 1, &error_msg);
    ASSERT(ret == MIMI_CONFIG_ERR_VALIDATION);
    ASSERT(error_msg != NULL);
    /* Error message should mention the missing key */
    ASSERT(strstr(error_msg, "missing_key") != NULL);
    
    printf("[error: %s] ", error_msg);
    free(error_msg);
    mimi_config_free(cfg);
}

/* Test validation with many required keys */
TEST(test_validate_many_keys)
{
    /* Config with many keys */
    const char *json = "{"
        "\"a\": 1, \"b\": 2, \"c\": 3, \"d\": 4, \"e\": 5,"
        "\"f\": 6, \"g\": 7, \"h\": 8, \"i\": 9, \"j\": 10"
    "}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 10, &error_msg);
    ASSERT(ret == MIMI_CONFIG_OK);
    
    mimi_config_free(cfg);
}

/* Test validation partial match */
TEST(test_validate_partial_match)
{
    const char *json = "{\"key1\": \"value1\", \"key2\": \"value2\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    const char *required[] = {"key1", "key2", "key3"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required, 3, &error_msg);
    ASSERT(ret == MIMI_CONFIG_ERR_VALIDATION);
    ASSERT(error_msg != NULL);
    ASSERT(strstr(error_msg, "key3") != NULL);
    
    free(error_msg);
    mimi_config_free(cfg);
}

int main(void)
{
    printf("=== libmimi-config Validation Tests ===\n\n");
    
    RUN_TEST(test_validate_all_keys_present);
    RUN_TEST(test_validate_missing_keys);
    RUN_TEST(test_validate_empty_required);
    RUN_TEST(test_validate_nested_keys);
    RUN_TEST(test_validate_nested_missing);
    RUN_TEST(test_validate_null_config);
    RUN_TEST(test_validate_duplicate_keys);
    RUN_TEST(test_validate_error_message_content);
    RUN_TEST(test_validate_many_keys);
    RUN_TEST(test_validate_partial_match);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
