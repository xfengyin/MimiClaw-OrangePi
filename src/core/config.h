/*
 * Configuration Management Header
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <json-c/json.h>

#define CONFIG_VERSION "1.0.0"
#define DEFAULT_CONFIG_PATH "/etc/mimiclaw/config.json"
#define MAX_PATH_LENGTH 256
#define MAX_STRING_LENGTH 1024

typedef struct {
    char name[MAX_STRING_LENGTH];
    char version[MAX_STRING_LENGTH];
    char log_level[16];
    char data_directory[MAX_PATH_LENGTH];
} system_config_t;

typedef struct {
    bool enabled;
    char ssid[MAX_STRING_LENGTH];
    char password[MAX_STRING_LENGTH];
    char country[4];
} wifi_config_t;

typedef struct {
    bool enabled;
    bool dhcp;
} ethernet_config_t;

typedef struct {
    wifi_config_t wifi;
    ethernet_config_t ethernet;
} network_config_t;

typedef struct {
    bool enabled;
    char bot_token[MAX_STRING_LENGTH];
    char allowed_users[MAX_STRING_LENGTH];
    char webhook_url[MAX_STRING_LENGTH];
} telegram_config_t;

typedef struct {
    char api_key[MAX_STRING_LENGTH];
    char model[MAX_STRING_LENGTH];
    int max_tokens;
    float temperature;
} anthropic_config_t;

typedef struct {
    bool enabled;
    char type[16];
    char host[MAX_STRING_LENGTH];
    int port;
    char username[MAX_STRING_LENGTH];
    char password[MAX_STRING_LENGTH];
} proxy_config_t;

typedef struct {
    char storage_type[16];
    char database_path[MAX_PATH_LENGTH];
    int max_sessions;
    int session_timeout;
    bool auto_save;
} memory_config_t;

typedef struct {
    bool enabled;
    int port;
    int max_connections;
} websocket_config_t;

typedef struct {
    bool enabled;
    int port;
    bool auth_required;
} http_config_t;

typedef struct {
    websocket_config_t websocket;
    http_config_t http;
} gateway_config_t;

typedef struct {
    char name[MAX_STRING_LENGTH];
    char language[8];
    char tone[16];
    char system_prompt[MAX_STRING_LENGTH * 4];
} personality_config_t;

typedef struct {
    system_config_t system;
    network_config_t network;
    telegram_config_t telegram;
    anthropic_config_t anthropic;
    proxy_config_t proxy;
    memory_config_t memory;
    gateway_config_t gateway;
    personality_config_t personality;
} app_config_t;

// Function prototypes
int config_load(const char *path, app_config_t *config);
int config_save(const char *path, app_config_t *config);
void config_set_defaults(app_config_t *config);
void config_free(app_config_t *config);
const char* config_get_string(json_object *obj, const char *key, const char *default_val);
int config_get_int(json_object *obj, const char *key, int default_val);
bool config_get_bool(json_object *obj, const char *key, bool default_val);

#endif // CONFIG_H