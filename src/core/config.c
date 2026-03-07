/*
 * Configuration Management Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

void config_set_defaults(app_config_t *config) {
    if (!config) return;
    
    // System defaults
    strncpy(config->system.name, "MimiClaw-OrangePi", MAX_STRING_LENGTH);
    strncpy(config->system.version, CONFIG_VERSION, MAX_STRING_LENGTH);
    strncpy(config->system.log_level, "info", 16);
    strncpy(config->system.data_directory, "/var/lib/mimiclaw", MAX_PATH_LENGTH);
    
    // Network defaults
    config->network.wifi.enabled = true;
    strncpy(config->network.wifi.ssid, "", MAX_STRING_LENGTH);
    strncpy(config->network.wifi.password, "", MAX_STRING_LENGTH);
    strncpy(config->network.wifi.country, "CN", 4);
    
    config->network.ethernet.enabled = true;
    config->network.ethernet.dhcp = true;
    
    // Telegram defaults
    config->telegram.enabled = true;
    strncpy(config->telegram.bot_token, "", MAX_STRING_LENGTH);
    strncpy(config->telegram.allowed_users, "", MAX_STRING_LENGTH);
    strncpy(config->telegram.webhook_url, "", MAX_STRING_LENGTH);
    
    // Anthropic defaults
    strncpy(config->anthropic.api_key, "", MAX_STRING_LENGTH);
    strncpy(config->anthropic.model, "claude-sonnet-4-5-20250929", MAX_STRING_LENGTH);
    config->anthropic.max_tokens = 4096;
    config->anthropic.temperature = 0.7;
    
    // Proxy defaults
    config->proxy.enabled = false;
    strncpy(config->proxy.type, "http", 16);
    strncpy(config->proxy.host, "127.0.0.1", MAX_STRING_LENGTH);
    config->proxy.port = 7890;
    strncpy(config->proxy.username, "", MAX_STRING_LENGTH);
    strncpy(config->proxy.password, "", MAX_STRING_LENGTH);
    
    // Memory defaults
    strncpy(config->memory.storage_type, "sqlite", 16);
    strncpy(config->memory.database_path, "/var/lib/mimiclaw/memory.db", MAX_PATH_LENGTH);
    config->memory.max_sessions = 100;
    config->memory.session_timeout = 3600;
    config->memory.auto_save = true;
    
    // Gateway defaults
    config->gateway.websocket.enabled = true;
    config->gateway.websocket.port = 18789;
    config->gateway.websocket.max_connections = 10;
    
    config->gateway.http.enabled = true;
    config->gateway.http.port = 8080;
    config->gateway.http.auth_required = true;
    
    // Personality defaults
    strncpy(config->personality.name, "Mimi", MAX_STRING_LENGTH);
    strncpy(config->personality.language, "zh-CN", 8);
    strncpy(config->personality.tone, "friendly", 16);
    strncpy(config->personality.system_prompt, 
        "你是一个运行在 OrangePi Zero3 上的 AI 助手。你乐于助人、友好、知识渊博。", 
        MAX_STRING_LENGTH * 4);
}

const char* config_get_string(json_object *obj, const char *key, const char *default_val) {
    json_object *val;
    if (json_object_object_get_ex(obj, key, &val)) {
        return json_object_get_string(val);
    }
    return default_val;
}

int config_get_int(json_object *obj, const char *key, int default_val) {
    json_object *val;
    if (json_object_object_get_ex(obj, key, &val)) {
        return json_object_get_int(val);
    }
    return default_val;
}

bool config_get_bool(json_object *obj, const char *key, bool default_val) {
    json_object *val;
    if (json_object_object_get_ex(obj, key, &val)) {
        return json_object_get_boolean(val);
    }
    return default_val;
}

int config_load(const char *path, app_config_t *config) {
    if (!path || !config) return -1;
    
    // Set defaults first
    config_set_defaults(config);
    
    // Read file
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Warning: Cannot open config file %s, using defaults\n", path);
        return 0; // Use defaults
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }
    
    size_t read_size = fread(buffer, 1, size, fp);
    (void)read_size; // Suppress warning
    buffer[size] = '\0';
    fclose(fp);
    
    // Parse JSON
    json_object *root = json_tokener_parse(buffer);
    free(buffer);
    
    if (!root) {
        fprintf(stderr, "Error: Failed to parse config file\n");
        return -1;
    }
    
    // Parse system config
    json_object *system_obj;
    if (json_object_object_get_ex(root, "system", &system_obj)) {
        strncpy(config->system.name, 
            config_get_string(system_obj, "name", config->system.name), MAX_STRING_LENGTH);
        strncpy(config->system.log_level, 
            config_get_string(system_obj, "log_level", config->system.log_level), 16);
        strncpy(config->system.data_directory, 
            config_get_string(system_obj, "data_directory", config->system.data_directory), MAX_PATH_LENGTH);
    }
    
    // Parse network config
    json_object *network_obj;
    if (json_object_object_get_ex(root, "network", &network_obj)) {
        json_object *wifi_obj;
        if (json_object_object_get_ex(network_obj, "wifi", &wifi_obj)) {
            config->network.wifi.enabled = config_get_bool(wifi_obj, "enabled", config->network.wifi.enabled);
            strncpy(config->network.wifi.ssid, 
                config_get_string(wifi_obj, "ssid", config->network.wifi.ssid), MAX_STRING_LENGTH);
            strncpy(config->network.wifi.password, 
                config_get_string(wifi_obj, "password", config->network.wifi.password), MAX_STRING_LENGTH);
        }
    }
    
    // Parse telegram config
    json_object *telegram_obj;
    if (json_object_object_get_ex(root, "telegram", &telegram_obj)) {
        config->telegram.enabled = config_get_bool(telegram_obj, "enabled", config->telegram.enabled);
        strncpy(config->telegram.bot_token, 
            config_get_string(telegram_obj, "bot_token", config->telegram.bot_token), MAX_STRING_LENGTH);
    }
    
    // Parse anthropic config
    json_object *anthropic_obj;
    if (json_object_object_get_ex(root, "anthropic", &anthropic_obj)) {
        strncpy(config->anthropic.api_key, 
            config_get_string(anthropic_obj, "api_key", config->anthropic.api_key), MAX_STRING_LENGTH);
        strncpy(config->anthropic.model, 
            config_get_string(anthropic_obj, "model", config->anthropic.model), MAX_STRING_LENGTH);
        config->anthropic.max_tokens = config_get_int(anthropic_obj, "max_tokens", config->anthropic.max_tokens);
        config->anthropic.temperature = config_get_int(anthropic_obj, "temperature", 
            (int)(config->anthropic.temperature * 10)) / 10.0;
    }
    
    // Parse memory config
    json_object *memory_obj;
    if (json_object_object_get_ex(root, "memory", &memory_obj)) {
        strncpy(config->memory.database_path, 
            config_get_string(memory_obj, "database_path", config->memory.database_path), MAX_PATH_LENGTH);
        config->memory.max_sessions = config_get_int(memory_obj, "max_sessions", config->memory.max_sessions);
    }
    
    // Parse gateway config
    json_object *gateway_obj;
    if (json_object_object_get_ex(root, "gateway", &gateway_obj)) {
        json_object *ws_obj;
        if (json_object_object_get_ex(gateway_obj, "websocket", &ws_obj)) {
            config->gateway.websocket.enabled = config_get_bool(ws_obj, "enabled", config->gateway.websocket.enabled);
            config->gateway.websocket.port = config_get_int(ws_obj, "port", config->gateway.websocket.port);
        }
        
        json_object *http_obj;
        if (json_object_object_get_ex(gateway_obj, "http", &http_obj)) {
            config->gateway.http.enabled = config_get_bool(http_obj, "enabled", config->gateway.http.enabled);
            config->gateway.http.port = config_get_int(http_obj, "port", config->gateway.http.port);
        }
    }
    
    // Parse personality config
    json_object *personality_obj;
    if (json_object_object_get_ex(root, "personality", &personality_obj)) {
        strncpy(config->personality.name, 
            config_get_string(personality_obj, "name", config->personality.name), MAX_STRING_LENGTH);
        strncpy(config->personality.system_prompt, 
            config_get_string(personality_obj, "system_prompt", config->personality.system_prompt), MAX_STRING_LENGTH * 4);
    }
    
    json_object_put(root);
    return 0;
}

int config_save(const char *path, app_config_t *config) {
    if (!path || !config) return -1;
    
    json_object *root = json_object_new_object();
    
    // System config
    json_object *system_obj = json_object_new_object();
    json_object_object_add(system_obj, "name", json_object_new_string(config->system.name));
    json_object_object_add(system_obj, "log_level", json_object_new_string(config->system.log_level));
    json_object_object_add(system_obj, "data_directory", json_object_new_string(config->system.data_directory));
    json_object_object_add(root, "system", system_obj);
    
    // Telegram config
    json_object *telegram_obj = json_object_new_object();
    json_object_object_add(telegram_obj, "enabled", json_object_new_boolean(config->telegram.enabled));
    json_object_object_add(telegram_obj, "bot_token", json_object_new_string(config->telegram.bot_token));
    json_object_object_add(root, "telegram", telegram_obj);
    
    // Anthropic config
    json_object *anthropic_obj = json_object_new_object();
    json_object_object_add(anthropic_obj, "api_key", json_object_new_string(config->anthropic.api_key));
    json_object_object_add(anthropic_obj, "model", json_object_new_string(config->anthropic.model));
    json_object_object_add(anthropic_obj, "max_tokens", json_object_new_int(config->anthropic.max_tokens));
    json_object_object_add(anthropic_obj, "temperature", json_object_new_double(config->anthropic.temperature));
    json_object_object_add(root, "anthropic", anthropic_obj);
    
    // Save to file
    FILE *fp = fopen(path, "w");
    if (!fp) {
        json_object_put(root);
        return -1;
    }
    
    fprintf(fp, "%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(fp);
    
    json_object_put(root);
    return 0;
}

void config_free(app_config_t *config) {
    // Nothing to free for now (all static buffers)
    (void)config;
}