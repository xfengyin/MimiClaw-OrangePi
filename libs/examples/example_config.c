/**
 * @file example_config.c
 * @brief Example: Using libmimi-config
 */

#include <stdio.h>
#include <stdlib.h>
#include "mimi_config.h"

int main(void)
{
    printf("=== MimiConfig Example ===\n\n");
    printf("Library version: %s\n\n", mimi_config_version());
    
    /* Load from JSON string */
    const char *json = 
        "{"
        "  \"app\": {"
        "    \"name\": \"MimiClaw\","
        "    \"version\": \"2.0.0\""
        "  },"
        "  \"api\": {"
        "    \"key\": \"test-key\","
        "    \"timeout\": 30000,"
        "    \"retry\": true"
        "  },"
        "  \"features\": ["
        "    \"chat\","
        "    \"memory\","
        "    \"tools\""
        "  ]"
        "}";
    
    mimi_config_t *cfg = mimi_config_load_json(json);
    if (cfg == NULL) {
        fprintf(stderr, "Load failed\n");
        return 1;
    }
    printf("✓ Config loaded from JSON\n");
    
    /* Get string value */
    char *app_name;
    if (mimi_config_get_string(cfg, "app.name", &app_name) == MIMI_CONFIG_OK) {
        printf("✓ app.name = %s\n", app_name);
        free(app_name);
    }
    
    /* Get nested string value */
    char *api_key;
    if (mimi_config_get_string(cfg, "api.key", &api_key) == MIMI_CONFIG_OK) {
        printf("✓ api.key = %s\n", api_key);
        free(api_key);
    }
    
    /* Get int value */
    int timeout;
    if (mimi_config_get_int(cfg, "api.timeout", &timeout) == MIMI_CONFIG_OK) {
        printf("✓ api.timeout = %d\n", timeout);
    }
    
    /* Get bool value */
    int retry;
    if (mimi_config_get_bool(cfg, "api.retry", &retry) == MIMI_CONFIG_OK) {
        printf("✓ api.retry = %s\n", retry ? "true" : "false");
    }
    
    /* Check key exists */
    if (mimi_config_has_key(cfg, "api.key")) {
        printf("✓ Key 'api.key' exists\n");
    }
    
    /* Get type */
    mimi_config_type_t type = mimi_config_get_type(cfg, "features");
    printf("✓ Type of 'features': %s\n", 
           type == MIMI_CONFIG_TYPE_ARRAY ? "ARRAY" : "OTHER");
    
    /* Validate required keys */
    const char *required[] = {"app.name", "api.key"};
    char *error_msg;
    int ret = mimi_config_validate(cfg, required, 2, &error_msg);
    if (ret == MIMI_CONFIG_OK) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
        free(error_msg);
    }
    
    /* Cleanup */
    mimi_config_free(cfg);
    printf("✓ Config freed\n");
    
    printf("\n=== Example Complete ===\n");
    return 0;
}
