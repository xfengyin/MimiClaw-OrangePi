# MimiClaw API Reference

Complete API reference for MimiClaw v2.0 libraries.

## Table of Contents

1. [libmimi-core](#libmimi-core)
2. [libmimi-memory](#libmimi-memory)
3. [libmimi-config](#libmimi-config)
4. [libmimi-tools](#libmimi-tools)

---

## libmimi-core

Core AI agent functionality.

### Types

```c
typedef struct mimi_core_ctx mimi_core_ctx_t;
typedef struct mimi_core_config mimi_core_config_t;
typedef struct mimi_core_response mimi_core_response_t;
```

### Lifecycle

```c
int mimi_core_init(mimi_core_ctx_t **ctx, const mimi_core_config_t *config);
int mimi_core_destroy(mimi_core_ctx_t *ctx);
```

### Chat

```c
int mimi_core_chat(mimi_core_ctx_t *ctx, const char *session_id,
                   const char *user_message, mimi_core_response_t *response);
void mimi_core_response_free(mimi_core_response_t *response);
```

### Context Management

```c
int mimi_core_set_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, const char *value);
int mimi_core_get_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *key, char **value);
int mimi_core_delete_context(mimi_core_ctx_t *ctx, const char *session_id,
                             const char *key);
int mimi_core_clear_context(mimi_core_ctx_t *ctx, const char *session_id);
```

### Session Management

```c
int mimi_core_session_create(mimi_core_ctx_t *ctx, const char *session_id);
int mimi_core_session_delete(mimi_core_ctx_t *ctx, const char *session_id);
int mimi_core_session_list(mimi_core_ctx_t *ctx, char ***sessions, int *count);
```

### Error Handling

```c
const char* mimi_core_strerror(int error);
const char* mimi_core_version(void);
```

---

## libmimi-memory

SQLite-based persistence layer.

### Types

```c
typedef struct mimi_mem_pool mimi_mem_pool_t;
typedef struct mimi_mem_config mimi_mem_config_t;
typedef struct mimi_mem_message mimi_mem_message_t;
typedef struct mimi_mem_entry mimi_mem_entry_t;
```

### Pool Management

```c
mimi_mem_pool_t* mimi_mem_pool_create(const mimi_mem_config_t *config);
void mimi_mem_pool_destroy(mimi_mem_pool_t *pool);
```

### Session Management

```c
int mimi_mem_session_create(mimi_mem_pool_t *pool, const char *session_id);
int mimi_mem_session_delete(mimi_mem_pool_t *pool, const char *session_id);
int mimi_mem_session_list(mimi_mem_pool_t *pool, char ***sessions, int *count);
int mimi_mem_session_exists(mimi_mem_pool_t *pool, const char *session_id);
```

### Message Storage

```c
int mimi_mem_message_append(mimi_mem_pool_t *pool, const char *session_id,
                            const char *role, const char *content, int64_t *msg_id);
int mimi_mem_message_query(mimi_mem_pool_t *pool, const char *session_id,
                           int limit, mimi_mem_message_t ***messages, int *count);
int mimi_mem_message_clear(mimi_mem_pool_t *pool, const char *session_id);
void mimi_mem_message_free(mimi_mem_message_t *msg);
```

### Key-Value Store

```c
int mimi_mem_write(mimi_mem_pool_t *pool, const char *key, const char *value);
int mimi_mem_read(mimi_mem_pool_t *pool, const char *key, char **value);
int mimi_mem_delete(mimi_mem_pool_t *pool, const char *key);
int mimi_mem_search(mimi_mem_pool_t *pool, const char *query,
                    mimi_mem_entry_t ***results, int *count);
void mimi_mem_entry_free(mimi_mem_entry_t *entry);
```

### Utility

```c
int mimi_mem_vacuum(mimi_mem_pool_t *pool);
const char* mimi_mem_last_error(mimi_mem_pool_t *pool);
const char* mimi_mem_strerror(int error);
const char* mimi_mem_version(void);
```

---

## libmimi-config

Configuration management with hot reload.

### Types

```c
typedef struct mimi_config mimi_config_t;
typedef struct mimi_config_options mimi_config_options_t;
typedef void (*mimi_config_reload_cb)(void *user_data, int status);
```

### Lifecycle

```c
mimi_config_t* mimi_config_load(const char *path);
mimi_config_t* mimi_config_load_json(const char *json);
void mimi_config_free(mimi_config_t *cfg);
```

### Reading Values

```c
int mimi_config_get_string(mimi_config_t *cfg, const char *key, char **value);
int mimi_config_get_int(mimi_config_t *cfg, const char *key, int *value);
int mimi_config_get_float(mimi_config_t *cfg, const char *key, double *value);
int mimi_config_get_bool(mimi_config_t *cfg, const char *key, int *value);
int mimi_config_has_key(mimi_config_t *cfg, const char *key);
mimi_config_type_t mimi_config_get_type(mimi_config_t *cfg, const char *key);
```

### Hot Reload

```c
int mimi_config_watch(mimi_config_t *cfg, const mimi_config_options_t *options);
int mimi_config_reload(mimi_config_t *cfg);
void mimi_config_unwatch(mimi_config_t *cfg);
```

### Validation

```c
int mimi_config_validate(mimi_config_t *cfg, const char **required_keys,
                         int key_count, char **error_msg);
```

### Utility

```c
const char* mimi_config_get_path(mimi_config_t *cfg);
int64_t mimi_config_get_mtime(mimi_config_t *cfg);
const char* mimi_config_strerror(int error);
const char* mimi_config_version(void);
```

---

## libmimi-tools

Plugin system and tool registry.

### Types

```c
typedef struct mimi_tool_registry mimi_tool_registry_t;
typedef struct mimi_plugin mimi_plugin_t;
typedef struct mimi_plugin_meta mimi_plugin_meta_t;
typedef struct mimi_tool_info mimi_tool_info_t;
```

### Registry Lifecycle

```c
mimi_tool_registry_t* mimi_registry_create(void);
void mimi_registry_destroy(mimi_tool_registry_t *reg);
```

### Tool Registration

```c
int mimi_registry_register_tool(mimi_tool_registry_t *reg,
                                const mimi_plugin_t *plugin,
                                mimi_tool_ctx_t *ctx);
int mimi_registry_unregister_tool(mimi_tool_registry_t *reg, const char *name);
```

### Plugin Loading

```c
int mimi_registry_load_plugin(mimi_tool_registry_t *reg, const char *so_path);
int mimi_registry_unload_plugin(mimi_tool_registry_t *reg, const char *name);
int mimi_registry_load_plugins_dir(mimi_tool_registry_t *reg, const char *dir_path);
const mimi_plugin_t* mimi_registry_get_plugin(mimi_tool_registry_t *reg, const char *name);
```

### Tool Execution

```c
int mimi_registry_exec(mimi_tool_registry_t *reg, const char *tool_name,
                       const char *input, char **output);
int mimi_registry_has_tool(mimi_tool_registry_t *reg, const char *name);
```

### Tool Information

```c
mimi_tool_info_t* mimi_registry_get_tool_info(mimi_tool_registry_t *reg, const char *name);
void mimi_tool_info_free(mimi_tool_info_t *info);
int mimi_registry_list(mimi_tool_registry_t *reg, mimi_tool_info_t ***tools, int *count);
```

### Built-in Tools

```c
int mimi_tools_register_time(mimi_tool_registry_t *reg);
int mimi_tools_register_echo(mimi_tool_registry_t *reg);
```

### Utility

```c
const char* mimi_tools_strerror(int error);
const char* mimi_tools_version(void);
```

---

## Error Codes

### libmimi-core

| Code | Name | Description |
|------|------|-------------|
| 0 | MIMI_CORE_OK | Success |
| -1 | MIMI_CORE_ERR_INVALID_ARG | Invalid argument |
| -2 | MIMI_CORE_ERR_NO_MEMORY | Memory allocation failed |
| -3 | MIMI_CORE_ERR_INIT_FAILED | Initialization failed |
| -4 | MIMI_CORE_ERR_NOT_FOUND | Resource not found |
| -5 | MIMI_CORE_ERR_API_ERROR | External API error |
| -6 | MIMI_CORE_ERR_TIMEOUT | Operation timeout |
| -7 | MIMI_CORE_ERR_SESSION_EXISTS | Session already exists |
| -8 | MIMI_CORE_ERR_SESSION_NOT_FOUND | Session not found |

### libmimi-memory

| Code | Name | Description |
|------|------|-------------|
| 0 | MIMI_MEMORY_OK | Success |
| -1 | MIMI_MEMORY_ERR_INVALID_ARG | Invalid argument |
| -2 | MIMI_MEMORY_ERR_NO_MEMORY | Memory allocation failed |
| -3 | MIMI_MEMORY_ERR_DB_ERROR | SQLite database error |
| -4 | MIMI_MEMORY_ERR_NOT_FOUND | Resource not found |
| -5 | MIMI_MEMORY_ERR_EXISTS | Resource already exists |
| -6 | MIMI_MEMORY_ERR_POOL_FULL | Connection pool is full |
| -7 | MIMI_MEMORY_ERR_INIT_FAILED | Initialization failed |

### libmimi-config

| Code | Name | Description |
|------|------|-------------|
| 0 | MIMI_CONFIG_OK | Success |
| -1 | MIMI_CONFIG_ERR_INVALID_ARG | Invalid argument |
| -2 | MIMI_CONFIG_ERR_NO_MEMORY | Memory allocation failed |
| -3 | MIMI_CONFIG_ERR_FILE_NOT_FOUND | Configuration file not found |
| -4 | MIMI_CONFIG_ERR_PARSE_ERROR | JSON/YAML parse error |
| -5 | MIMI_CONFIG_ERR_TYPE_MISMATCH | Type mismatch |
| -6 | MIMI_CONFIG_ERR_NOT_FOUND | Key not found |
| -7 | MIMI_CONFIG_ERR_VALIDATION | Validation failed |
| -8 | MIMI_CONFIG_ERR_IO | I/O error |

### libmimi-tools

| Code | Name | Description |
|------|------|-------------|
| 0 | MIMI_TOOLS_OK | Success |
| -1 | MIMI_TOOLS_ERR_INVALID_ARG | Invalid argument |
| -2 | MIMI_TOOLS_ERR_NO_MEMORY | Memory allocation failed |
| -3 | MIMI_TOOLS_ERR_NOT_FOUND | Tool not found |
| -4 | MIMI_TOOLS_ERR_EXISTS | Tool already exists |
| -5 | MIMI_TOOLS_ERR_LOAD_FAILED | Plugin load failed |
| -6 | MIMI_TOOLS_ERR_EXEC_FAILED | Tool execution failed |
| -7 | MIMI_TOOLS_ERR_INIT_FAILED | Tool initialization failed |
| -8 | MIMI_TOOLS_ERR_UNLOAD_FAILED | Plugin unload failed |

---

For more detailed documentation, see the [Developer Guide](DEVELOPER-GUIDE.md) or generate HTML docs with Doxygen.
