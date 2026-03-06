/*
 * Configuration Test
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/core/config.h"

void test_config_defaults() {
    app_config_t config;
    config_set_defaults(&config);
    
    assert(strcmp(config.system.name, "MimiClaw-OrangePi") == 0);
    assert(config.gateway.websocket.port == 18789);
    assert(config.anthropic.max_tokens == 4096);
    
    printf("✓ Config defaults test passed\n");
}

void test_config_get_functions() {
    const char *json_str = "{\"name\": \"test\", \"value\": 42, \"enabled\": true}";
    json_object *obj = json_tokener_parse(json_str);
    
    assert(strcmp(config_get_string(obj, "name", "default"), "test") == 0);
    assert(config_get_int(obj, "value", 0) == 42);
    assert(config_get_bool(obj, "enabled", false) == true);
    assert(strcmp(config_get_string(obj, "missing", "default"), "default") == 0);
    
    json_object_put(obj);
    printf("✓ Config get functions test passed\n");
}

int main() {
    printf("Running configuration tests...\n");
    
    test_config_defaults();
    test_config_get_functions();
    
    printf("\nAll tests passed!\n");
    return 0;
}