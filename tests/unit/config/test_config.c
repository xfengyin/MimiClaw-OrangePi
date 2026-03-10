/**
 * @file test_config.c
 * @brief Unit tests for Config Library - Basic Operations
 * @coverage mimi_config load, parse, get operations
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

static const char *TEST_CONFIG_PATH = "/tmp/test_mimi_config.json";

/* Test load from JSON string */
TEST(test_config_load_json)
{
    const char *json = "{\"name\": \"test\", \"value\": 42, \"enabled\": true}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    /* Get string */
    char *name = NULL;
    ASSERT(mimi_config_get_string(cfg, "name", &name) == MIMI_CONFIG_OK);
    ASSERT(strcmp(name, "test") == 0);
    free(name);
    
    /* Get int */
    int value = 0;
    ASSERT(mimi_config_get_int(cfg, "value", &value) == MIMI_CONFIG_OK);
    ASSERT(value == 42);
    
    /* Get bool */
    int enabled = 0;
    ASSERT(mimi_config_get_bool(cfg, "enabled", &enabled) == MIMI_CONFIG_OK);
    ASSERT(enabled == 1);
    
    mimi_config_free(cfg);
}

/* Test load from file */
TEST(test_config_load_file)
{
    /* Create test file */
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"database\": {\"host\": \"localhost\", \"port\": 5432}}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    /* Get nested value */
    char *host = NULL;
    ASSERT(mimi_config_get_string(cfg, "database.host", &host) == MIMI_CONFIG_OK);
    ASSERT(strcmp(host, "localhost") == 0);
    free(host);
    
    int port = 0;
    ASSERT(mimi_config_get_int(cfg, "database.port", &port) == MIMI_CONFIG_OK);
    ASSERT(port == 5432);
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

/* Test load non-existent file */
TEST(test_config_load_nonexistent)
{
    mimi_config_t *cfg = mimi_config_load("/nonexistent/path/config.json");
    ASSERT(cfg == NULL);
}

/* Test get float */
TEST(test_config_get_float)
{
    const char *json = "{\"pi\": 3.14159, \"ratio\": 0.5}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    double pi = 0.0;
    ASSERT(mimi_config_get_float(cfg, "pi", &pi) == MIMI_CONFIG_OK);
    ASSERT(pi > 3.14 && pi < 3.15);
    
    double ratio = 0.0;
    ASSERT(mimi_config_get_float(cfg, "ratio", &ratio) == MIMI_CONFIG_OK);
    ASSERT(ratio == 0.5);
    
    mimi_config_free(cfg);
}

/* Test get non-existent key */
TEST(test_config_get_nonexistent)
{
    const char *json = "{\"key\": \"value\"}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    char *value = NULL;
    int ret = mimi_config_get_string(cfg, "nonexistent", &value);
    ASSERT(ret == MIMI_CONFIG_ERR_NOT_FOUND);
    ASSERT(value == NULL);
    
    mimi_config_free(cfg);
}

/* Test has_key */
TEST(test_config_has_key)
{
    const char *json = "{\"exists\": true, \"nested\": {\"key\": \"value\"}}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    ASSERT(mimi_config_has_key(cfg, "exists") == 1);
    ASSERT(mimi_config_has_key(cfg, "nested.key") == 1);
    ASSERT(mimi_config_has_key(cfg, "nonexistent") == 0);
    
    mimi_config_free(cfg);
}

/* Test get_type */
TEST(test_config_get_type)
{
    const char *json = "{\"str\": \"hello\", \"num\": 42, \"flt\": 3.14, \"bool\": true, \"arr\": [1,2,3], \"obj\": {}}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    ASSERT(mimi_config_get_type(cfg, "str") == MIMI_CONFIG_TYPE_STRING);
    ASSERT(mimi_config_get_type(cfg, "num") == MIMI_CONFIG_TYPE_INTEGER);
    ASSERT(mimi_config_get_type(cfg, "flt") == MIMI_CONFIG_TYPE_FLOAT);
    ASSERT(mimi_config_get_type(cfg, "bool") == MIMI_CONFIG_TYPE_BOOLEAN);
    ASSERT(mimi_config_get_type(cfg, "arr") == MIMI_CONFIG_TYPE_ARRAY);
    ASSERT(mimi_config_get_type(cfg, "obj") == MIMI_CONFIG_TYPE_OBJECT);
    ASSERT(mimi_config_get_type(cfg, "nonexistent") == MIMI_CONFIG_TYPE_NULL);
    
    mimi_config_free(cfg);
}

/* Test null config handling */
TEST(test_config_null_handling)
{
    /* Free NULL config */
    mimi_config_free(NULL);
    
    /* Operations on NULL config */
    char *value = NULL;
    ASSERT(mimi_config_get_string(NULL, "key", &value) != MIMI_CONFIG_OK);
    
    int ivalue = 0;
    ASSERT(mimi_config_get_int(NULL, "key", &ivalue) != MIMI_CONFIG_OK);
    
    double fvalue = 0.0;
    ASSERT(mimi_config_get_float(NULL, "key", &fvalue) != MIMI_CONFIG_OK);
    
    int bvalue = 0;
    ASSERT(mimi_config_get_bool(NULL, "key", &bvalue) != MIMI_CONFIG_OK);
}

/* Test version and error functions */
TEST(test_config_utility_functions)
{
    ASSERT(mimi_config_version() != NULL);
    ASSERT(mimi_config_strerror(MIMI_CONFIG_OK) != NULL);
    ASSERT(mimi_config_strerror(MIMI_CONFIG_ERR_INVALID_ARG) != NULL);
    ASSERT(mimi_config_strerror(-999) != NULL);
}

/* Test complex nested config */
TEST(test_config_nested_structure)
{
    const char *json = "{"
        "\"server\": {"
            "\"host\": \"api.example.com\","
            "\"port\": 8080,"
            "\"ssl\": {"
                "\"enabled\": true,"
                "\"cert\": \"/path/to/cert\""
            "}"
        "}"
    "}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    ASSERT(cfg != NULL);
    
    char *host = NULL;
    ASSERT(mimi_config_get_string(cfg, "server.host", &host) == MIMI_CONFIG_OK);
    ASSERT(strcmp(host, "api.example.com") == 0);
    free(host);
    
    int port = 0;
    ASSERT(mimi_config_get_int(cfg, "server.port", &port) == MIMI_CONFIG_OK);
    ASSERT(port == 8080);
    
    int ssl_enabled = 0;
    ASSERT(mimi_config_get_bool(cfg, "server.ssl.enabled", &ssl_enabled) == MIMI_CONFIG_OK);
    ASSERT(ssl_enabled == 1);
    
    char *cert = NULL;
    ASSERT(mimi_config_get_string(cfg, "server.ssl.cert", &cert) == MIMI_CONFIG_OK);
    ASSERT(strcmp(cert, "/path/to/cert") == 0);
    free(cert);
    
    mimi_config_free(cfg);
}

/* Test get path and mtime */
TEST(test_config_path_mtime)
{
    /* Create test file */
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    ASSERT(f != NULL);
    fprintf(f, "{\"test\": true}");
    fclose(f);
    
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    ASSERT(cfg != NULL);
    
    const char *path = mimi_config_get_path(cfg);
    ASSERT(path != NULL);
    ASSERT(strstr(path, "test_mimi_config.json") != NULL);
    
    int64_t mtime = mimi_config_get_mtime(cfg);
    ASSERT(mtime > 0);
    
    mimi_config_free(cfg);
    remove(TEST_CONFIG_PATH);
}

int main(void)
{
    printf("=== libmimi-config Basic Tests ===\n\n");
    
    RUN_TEST(test_config_load_json);
    RUN_TEST(test_config_load_file);
    RUN_TEST(test_config_load_nonexistent);
    RUN_TEST(test_config_get_float);
    RUN_TEST(test_config_get_nonexistent);
    RUN_TEST(test_config_has_key);
    RUN_TEST(test_config_get_type);
    RUN_TEST(test_config_null_handling);
    RUN_TEST(test_config_utility_functions);
    RUN_TEST(test_config_nested_structure);
    RUN_TEST(test_config_path_mtime);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
