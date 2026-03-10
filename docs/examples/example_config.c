/**
 * @file example_config.c
 * @brief Configuration management example
 * 
 * Demonstrates:
 * - Loading configuration from file
 * - Loading configuration from JSON string
 * - Reading different value types
 * - Hot reload functionality
 * - Configuration validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mimi_config.h"

static const char *TEST_CONFIG_PATH = "/tmp/mimi_example_config.json";

/* Create a sample config file */
static void create_sample_config(void)
{
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"app\": {\n");
    fprintf(f, "    \"name\": \"MimiClaw Example\",\n");
    fprintf(f, "    \"version\": \"2.0.0\",\n");
    fprintf(f, "    \"debug\": true\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"server\": {\n");
    fprintf(f, "    \"host\": \"localhost\",\n");
    fprintf(f, "    \"port\": 8080,\n");
    fprintf(f, "    \"timeout\": 30.5\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"features\": {\n");
    fprintf(f, "    \"plugins\": true,\n");
    fprintf(f, "    \"logging\": true,\n");
    fprintf(f, "    \"cache\": false\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    
    fclose(f);
}

int main(void)
{
    printf("=== MimiClaw Configuration Example ===\n\n");
    
    /* Create sample config file */
    create_sample_config();
    
    /* Load configuration from file */
    printf("--- Loading Configuration from File ---\n");
    mimi_config_t *cfg = mimi_config_load(TEST_CONFIG_PATH);
    if (!cfg) {
        fprintf(stderr, "Failed to load config from %s\n", TEST_CONFIG_PATH);
        return 1;
    }
    printf("✓ Configuration loaded from: %s\n\n", mimi_config_get_path(cfg));
    
    /* Read string value */
    printf("--- Reading String Values ---\n");
    char *value = NULL;
    
    if (mimi_config_get_string(cfg, "app.name", &value) == MIMI_CONFIG_OK) {
        printf("app.name: %s\n", value);
        free(value);
    }
    
    if (mimi_config_get_string(cfg, "server.host", &value) == MIMI_CONFIG_OK) {
        printf("server.host: %s\n", value);
        free(value);
    }
    
    /* Read integer value */
    printf("\n--- Reading Integer Values ---\n");
    int int_value = 0;
    
    if (mimi_config_get_int(cfg, "server.port", &int_value) == MIMI_CONFIG_OK) {
        printf("server.port: %d\n", int_value);
    }
    
    /* Read float value */
    printf("\n--- Reading Float Values ---\n");
    double float_value = 0.0;
    
    if (mimi_config_get_float(cfg, "server.timeout", &float_value) == MIMI_CONFIG_OK) {
        printf("server.timeout: %.2f\n", float_value);
    }
    
    /* Read boolean value */
    printf("\n--- Reading Boolean Values ---\n");
    int bool_value = 0;
    
    if (mimi_config_get_bool(cfg, "app.debug", &bool_value) == MIMI_CONFIG_OK) {
        printf("app.debug: %s\n", bool_value ? "true" : "false");
    }
    
    if (mimi_config_get_bool(cfg, "features.cache", &bool_value) == MIMI_CONFIG_OK) {
        printf("features.cache: %s\n", bool_value ? "true" : "false");
    }
    
    /* Check key existence */
    printf("\n--- Checking Key Existence ---\n");
    printf("Has 'app.name': %s\n", mimi_config_has_key(cfg, "app.name") ? "Yes" : "No");
    printf("Has 'app.author': %s\n", mimi_config_has_key(cfg, "app.author") ? "Yes" : "No");
    printf("Has 'server.host': %s\n", mimi_config_has_key(cfg, "server.host") ? "Yes" : "No");
    
    /* Get value type */
    printf("\n--- Getting Value Types ---\n");
    const char *type_names[] = {"NULL", "STRING", "INTEGER", "FLOAT", "BOOLEAN", "ARRAY", "OBJECT"};
    
    mimi_config_type_t type = mimi_config_get_type(cfg, "app.name");
    printf("app.name type: %s\n", type_names[type]);
    
    type = mimi_config_get_type(cfg, "server.port");
    printf("server.port type: %s\n", type_names[type]);
    
    type = mimi_config_get_type(cfg, "features");
    printf("features type: %s\n", type_names[type]);
    
    /* Load from JSON string */
    printf("\n--- Loading from JSON String ---\n");
    const char *json = "{\"key\": \"value\", \"number\": 42, \"enabled\": true}";
    mimi_config_t *cfg_json = mimi_config_load_json(json);
    if (cfg_json) {
        printf("✓ Configuration loaded from JSON string\n");
        
        if (mimi_config_get_string(cfg_json, "key", &value) == MIMI_CONFIG_OK) {
            printf("key: %s\n", value);
            free(value);
        }
        
        mimi_config_free(cfg_json);
    }
    
    /* Validate configuration */
    printf("\n--- Validating Configuration ---\n");
    const char *required_keys[] = {"app.name", "server.host", "server.port"};
    char *error_msg = NULL;
    
    int ret = mimi_config_validate(cfg, required_keys, 3, &error_msg);
    if (ret == MIMI_CONFIG_OK) {
        printf("✓ Configuration is valid\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
        free(error_msg);
    }
    
    /* Test hot reload */
    printf("\n--- Testing Hot Reload ---\n");
    printf("Initial version: ");
    if (mimi_config_get_string(cfg, "app.version", &value) == MIMI_CONFIG_OK) {
        printf("%s\n", value);
        free(value);
    }
    
    /* Modify config file */
    FILE *f = fopen(TEST_CONFIG_PATH, "w");
    if (f) {
        fprintf(f, "{\"app\": {\"name\": \"MimiClaw Example\", \"version\": \"2.1.0\"}, ");
        fprintf(f, "\"server\": {\"host\": \"localhost\", \"port\": 8080, \"timeout\": 30.5}, ");
        fprintf(f, "\"features\": {\"plugins\": true, \"logging\": true, \"cache\": false}}");
        fclose(f);
    }
    
    /* Reload */
    if (mimi_config_reload(cfg) == MIMI_CONFIG_OK) {
        printf("✓ Configuration reloaded\n");
        printf("New version: ");
        if (mimi_config_get_string(cfg, "app.version", &value) == MIMI_CONFIG_OK) {
            printf("%s\n", value);
            free(value);
        }
    }
    
    /* Get modification time */
    printf("\n--- File Information ---\n");
    printf("Config path: %s\n", mimi_config_get_path(cfg));
    printf("Last modified: %ld ms\n", (long)mimi_config_get_mtime(cfg));
    
    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    mimi_config_free(cfg);
    printf("Configuration freed\n");
    
    /* Remove test file */
    remove(TEST_CONFIG_PATH);
    printf("Test file removed\n");
    
    printf("\nDone!\n");
    
    return 0;
}

/*
 * Compile:
 * gcc -I../../libs/libmimi-config/include \
 *     example_config.c \
 *     -L../../libs/build/lib \
 *     -lmimi-config \
 *     -o example_config
 *
 * Run:
 * ./example_config
 */
