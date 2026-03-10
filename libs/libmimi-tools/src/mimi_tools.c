/**
 * @file mimi_tools.c
 * @brief MimiClaw Tools Library - Complete Implementation
 * @version 2.0.0
 * 
 * Combined implementation of registry and plugin loader.
 */

#define _GNU_SOURCE

#include "mimi_tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

/**
 * @brief Internal tool entry
 */
typedef struct mimi_tool_entry {
    char *name;
    char *description;
    char *version;
    char *author;
    mimi_plugin_t plugin;
    mimi_tool_ctx_t *ctx;
    void *dl_handle;           /**< Dynamic library handle (if plugin) */
    int is_builtin;
    char *plugin_path;
    struct mimi_tool_entry *next;
} mimi_tool_entry_t;

/**
 * @brief Internal registry structure
 */
struct mimi_tool_registry {
    mimi_tool_entry_t *tools;
    int count;
};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static mimi_tool_entry_t* find_tool(mimi_tool_registry_t *reg, const char *name)
{
    if (reg == NULL || name == NULL) {
        return NULL;
    }
    
    mimi_tool_entry_t *entry = reg->tools;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static mimi_tool_entry_t* create_entry(const mimi_plugin_t *plugin, mimi_tool_ctx_t *ctx)
{
    if (plugin == NULL || plugin->meta.name == NULL) {
        return NULL;
    }
    
    mimi_tool_entry_t *entry = (mimi_tool_entry_t*)calloc(1, sizeof(mimi_tool_entry_t));
    if (entry == NULL) {
        return NULL;
    }
    
    entry->name = strdup(plugin->meta.name);
    entry->description = plugin->meta.description ? strdup(plugin->meta.description) : NULL;
    entry->version = plugin->meta.version ? strdup(plugin->meta.version) : NULL;
    entry->author = plugin->meta.author ? strdup(plugin->meta.author) : NULL;
    entry->ctx = ctx;
    entry->plugin = *plugin;
    entry->dl_handle = NULL;
    entry->is_builtin = (ctx != NULL) ? 1 : 0;
    entry->plugin_path = NULL;
    entry->next = NULL;
    
    if (entry->name == NULL) {
        free(entry);
        return NULL;
    }
    
    return entry;
}

static void free_entry(mimi_tool_entry_t *entry)
{
    if (entry == NULL) {
        return;
    }
    
    /* Call destroy if available */
    if (entry->plugin.destroy != NULL && entry->ctx != NULL) {
        entry->plugin.destroy(entry->ctx);
    }
    
    /* Close dynamic library */
    if (entry->dl_handle != NULL) {
        dlclose(entry->dl_handle);
    }
    
    free(entry->name);
    free(entry->description);
    free(entry->version);
    free(entry->author);
    free(entry->plugin_path);
    free(entry);
}

/* ============================================================================
 * Registry Lifecycle
 * ============================================================================ */

mimi_tool_registry_t* mimi_registry_create(void)
{
    mimi_tool_registry_t *reg = (mimi_tool_registry_t*)calloc(1, sizeof(mimi_tool_registry_t));
    if (reg == NULL) {
        return NULL;
    }
    
    reg->tools = NULL;
    reg->count = 0;
    return reg;
}

void mimi_registry_destroy(mimi_tool_registry_t *reg)
{
    if (reg == NULL) {
        return;
    }
    
    mimi_tool_entry_t *entry = reg->tools;
    while (entry != NULL) {
        mimi_tool_entry_t *next = entry->next;
        free_entry(entry);
        entry = next;
    }
    
    free(reg);
}

/* ============================================================================
 * Built-in Tool Registration
 * ============================================================================ */

int mimi_registry_register_tool(mimi_tool_registry_t *reg,
                                const mimi_plugin_t *plugin,
                                mimi_tool_ctx_t *ctx)
{
    if (reg == NULL || plugin == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    /* Check if already exists */
    if (find_tool(reg, plugin->meta.name) != NULL) {
        return MIMI_TOOLS_ERR_EXISTS;
    }
    
    mimi_tool_entry_t *entry = create_entry(plugin, ctx);
    if (entry == NULL) {
        return MIMI_TOOLS_ERR_NO_MEMORY;
    }
    
    /* Add to registry */
    entry->next = reg->tools;
    reg->tools = entry;
    reg->count++;
    
    /* Initialize tool */
    if (entry->plugin.init != NULL && entry->ctx != NULL) {
        int ret = entry->plugin.init(entry->ctx);
        if (ret != 0) {
            mimi_registry_unregister_tool(reg, plugin->meta.name);
            return MIMI_TOOLS_ERR_INIT_FAILED;
        }
    }
    
    return MIMI_TOOLS_OK;
}

int mimi_registry_unregister_tool(mimi_tool_registry_t *reg, const char *name)
{
    if (reg == NULL || name == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    mimi_tool_entry_t **prev = &reg->tools;
    mimi_tool_entry_t *entry = reg->tools;
    
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            *prev = entry->next;
            free_entry(entry);
            reg->count--;
            return MIMI_TOOLS_OK;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    return MIMI_TOOLS_ERR_NOT_FOUND;
}

/* ============================================================================
 * Plugin Loading
 * ============================================================================ */

int mimi_registry_load_plugin(mimi_tool_registry_t *reg, const char *so_path)
{
    if (reg == NULL || so_path == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    /* Load shared library */
    void *dl_handle = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
    if (dl_handle == NULL) {
        fprintf(stderr, "[mimi-tools] dlopen failed: %s\n", dlerror());
        return MIMI_TOOLS_ERR_LOAD_FAILED;
    }
    
    /* Clear any existing error */
    dlerror();
    
    /* Get plugin interface */
    typedef const mimi_plugin_t* (*get_plugin_fn)(void);
    /* POSIX workaround for function pointer cast */
    union {
        void *p;
        get_plugin_fn f;
    } sym;
    sym.p = dlsym(dl_handle, "get_tool_plugin");
    get_plugin_fn get_plugin = sym.f;
    
    const char *error = dlerror();
    if (error != NULL || get_plugin == NULL) {
        fprintf(stderr, "[mimi-tools] dlsym failed: %s\n", error ? error : "no get_tool_plugin symbol");
        dlclose(dl_handle);
        return MIMI_TOOLS_ERR_LOAD_FAILED;
    }
    
    const mimi_plugin_t *plugin = get_plugin();
    if (plugin == NULL || plugin->meta.name == NULL) {
        dlclose(dl_handle);
        return MIMI_TOOLS_ERR_LOAD_FAILED;
    }
    
    /* Check if already exists */
    if (find_tool(reg, plugin->meta.name) != NULL) {
        dlclose(dl_handle);
        return MIMI_TOOLS_ERR_EXISTS;
    }
    
    /* Create entry */
    mimi_tool_entry_t *entry = create_entry(plugin, NULL);
    if (entry == NULL) {
        dlclose(dl_handle);
        return MIMI_TOOLS_ERR_NO_MEMORY;
    }
    
    entry->dl_handle = dl_handle;
    entry->is_builtin = 0;
    entry->plugin_path = strdup(so_path);
    
    /* Add to registry */
    entry->next = reg->tools;
    reg->tools = entry;
    reg->count++;
    
    /* Initialize tool (optional for plugins) */
    if (entry->plugin.init != NULL) {
        int ret = entry->plugin.init(entry->ctx);
        if (ret != 0) {
            mimi_registry_unregister_tool(reg, plugin->meta.name);
            return MIMI_TOOLS_ERR_INIT_FAILED;
        }
    }
    
    fprintf(stderr, "[mimi-tools] Loaded plugin: %s v%s from %s\n", 
            plugin->meta.name, plugin->meta.version ? plugin->meta.version : "?.?.?", so_path);
    
    return MIMI_TOOLS_OK;
}

int mimi_registry_unload_plugin(mimi_tool_registry_t *reg, const char *name)
{
    return mimi_registry_unregister_tool(reg, name);
}

const mimi_plugin_t* mimi_registry_get_plugin(mimi_tool_registry_t *reg, const char *name)
{
    mimi_tool_entry_t *entry = find_tool(reg, name);
    if (entry == NULL) {
        return NULL;
    }
    return &entry->plugin;
}

int mimi_registry_load_plugins_dir(mimi_tool_registry_t *reg, const char *dir_path)
{
    if (reg == NULL || dir_path == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "[mimi-tools] Cannot open plugins directory: %s\n", dir_path);
        return MIMI_TOOLS_ERR_NOT_FOUND;
    }
    
    int loaded_count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            if (strstr(entry->d_name, ".so") != NULL) {
                /* Build full path */
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                
                /* Try to load */
                int ret = mimi_registry_load_plugin(reg, full_path);
                if (ret == MIMI_TOOLS_OK) {
                    loaded_count++;
                }
            }
        }
    }
    
    closedir(dir);
    return loaded_count;
}

/* ============================================================================
 * Tool Execution
 * ============================================================================ */

int mimi_registry_exec(mimi_tool_registry_t *reg, const char *tool_name,
                       const char *input, char **output)
{
    if (reg == NULL || tool_name == NULL || output == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    mimi_tool_entry_t *entry = find_tool(reg, tool_name);
    if (entry == NULL) {
        return MIMI_TOOLS_ERR_NOT_FOUND;
    }
    
    if (entry->plugin.exec == NULL) {
        return MIMI_TOOLS_ERR_EXEC_FAILED;
    }
    
    *output = NULL;
    int ret = entry->plugin.exec(entry->ctx, input, output);
    
    if (ret != 0 || *output == NULL) {
        return MIMI_TOOLS_ERR_EXEC_FAILED;
    }
    
    return MIMI_TOOLS_OK;
}

int mimi_registry_has_tool(mimi_tool_registry_t *reg, const char *name)
{
    return (find_tool(reg, name) != NULL) ? 1 : 0;
}

mimi_tool_info_t* mimi_registry_get_tool_info(mimi_tool_registry_t *reg, const char *name)
{
    mimi_tool_entry_t *entry = find_tool(reg, name);
    if (entry == NULL) {
        return NULL;
    }
    
    mimi_tool_info_t *info = (mimi_tool_info_t*)calloc(1, sizeof(mimi_tool_info_t));
    if (info == NULL) {
        return NULL;
    }
    
    info->name = strdup(entry->name);
    info->description = entry->description ? strdup(entry->description) : NULL;
    info->version = entry->version ? strdup(entry->version) : NULL;
    info->author = entry->author ? strdup(entry->author) : NULL;
    info->is_builtin = entry->is_builtin;
    info->plugin_path = entry->plugin_path ? strdup(entry->plugin_path) : NULL;
    
    if (info->name == NULL) {
        mimi_tool_info_free(info);
        return NULL;
    }
    
    return info;
}

void mimi_tool_info_free(mimi_tool_info_t *info)
{
    if (info == NULL) {
        return;
    }
    
    free(info->name);
    free(info->description);
    free(info->version);
    free(info->author);
    free(info->plugin_path);
    free(info);
}

/* ============================================================================
 * Tool Listing
 * ============================================================================ */

int mimi_registry_list(mimi_tool_registry_t *reg, mimi_tool_info_t ***tools, int *count)
{
    if (reg == NULL || tools == NULL || count == NULL) {
        return MIMI_TOOLS_ERR_INVALID_ARG;
    }
    
    if (reg->count == 0) {
        *tools = NULL;
        *count = 0;
        return MIMI_TOOLS_OK;
    }
    
    *tools = (mimi_tool_info_t**)malloc(reg->count * sizeof(mimi_tool_info_t*));
    if (*tools == NULL) {
        return MIMI_TOOLS_ERR_NO_MEMORY;
    }
    
    mimi_tool_entry_t *entry = reg->tools;
    for (int i = 0; i < reg->count; i++) {
        (*tools)[i] = mimi_registry_get_tool_info(reg, entry->name);
        if ((*tools)[i] == NULL) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) {
                mimi_tool_info_free((*tools)[j]);
            }
            free(*tools);
            *tools = NULL;
            *count = 0;
            return MIMI_TOOLS_ERR_NO_MEMORY;
        }
        entry = entry->next;
    }
    
    *count = reg->count;
    return MIMI_TOOLS_OK;
}

/* ============================================================================
 * Built-in Tools Implementation
 * ============================================================================ */

/* --- Time Tool --- */

typedef struct {
    int initialized;
} time_tool_ctx_t;

static int time_tool_init(mimi_tool_ctx_t *ctx)
{
    time_tool_ctx_t *tctx = (time_tool_ctx_t*)ctx;
    tctx->initialized = 1;
    return 0;
}

static int time_tool_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    (void)ctx;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char buffer[256];
    
    /* Parse input for format selection */
    const char *fmt = "iso";
    if (input != NULL && strlen(input) > 0) {
        if (strstr(input, "date") != NULL) fmt = "date";
        else if (strstr(input, "time") != NULL) fmt = "time";
        else if (strstr(input, "unix") != NULL) fmt = "unix";
    }
    
    if (strcmp(fmt, "unix") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"timestamp\":%ld}", (long)now);
    } else if (strcmp(fmt, "date") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"date\":\"%04d-%02d-%02d\"}",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    } else if (strcmp(fmt, "time") == 0) {
        snprintf(buffer, sizeof(buffer), "{\"time\":\"%02d:%02d:%02d\"}",
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        snprintf(buffer, sizeof(buffer),
            "{\"timestamp\":%ld,\"iso\":\"%04d-%02d-%02dT%02d:%02d:%02d+08:00\",\"human\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
            (long)now,
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec
        );
    }
    
    *output = strdup(buffer);
    return (*output != NULL) ? 0 : -1;
}

static int time_tool_destroy(mimi_tool_ctx_t *ctx)
{
    time_tool_ctx_t *tctx = (time_tool_ctx_t*)ctx;
    tctx->initialized = 0;
    return 0;
}

static const mimi_plugin_t time_tool_plugin = {
    .meta = {
        .name = "time",
        .version = "1.0.0",
        .description = "Get current date and time in various formats",
        .author = "MimiClaw Project"
    },
    .init = time_tool_init,
    .exec = time_tool_exec,
    .destroy = time_tool_destroy
};

int mimi_tools_register_time(mimi_tool_registry_t *reg)
{
    time_tool_ctx_t *ctx = (time_tool_ctx_t*)calloc(1, sizeof(time_tool_ctx_t));
    if (ctx == NULL) {
        return MIMI_TOOLS_ERR_NO_MEMORY;
    }
    
    return mimi_registry_register_tool(reg, &time_tool_plugin, ctx);
}

/* --- Echo Tool --- */

typedef struct {
    int initialized;
} echo_tool_ctx_t;

static int echo_tool_init(mimi_tool_ctx_t *ctx)
{
    echo_tool_ctx_t *ectx = (echo_tool_ctx_t*)ctx;
    ectx->initialized = 1;
    return 0;
}

static int echo_tool_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    (void)ctx;
    
    if (input == NULL) {
        *output = strdup("{\"echo\":null}");
    } else {
        /* Simple JSON escape */
        size_t len = strlen(input);
        char *escaped = (char*)malloc(len * 2 + 16);
        if (escaped == NULL) {
            return -1;
        }
        
        char *p = escaped;
        p += sprintf(p, "{\"echo\":\"");
        for (size_t i = 0; i < len; i++) {
            char c = input[i];
            if (c == '"' || c == '\\') {
                *p++ = '\\';
            } else if (c == '\n') {
                *p++ = '\\';
                c = 'n';
            } else if (c == '\r') {
                *p++ = '\\';
                c = 'r';
            } else if (c == '\t') {
                *p++ = '\\';
                c = 't';
            }
            *p++ = c;
        }
        strcpy(p, "\"}");
        
        *output = escaped;
    }
    
    return (*output != NULL) ? 0 : -1;
}

static int echo_tool_destroy(mimi_tool_ctx_t *ctx)
{
    echo_tool_ctx_t *ectx = (echo_tool_ctx_t*)ctx;
    ectx->initialized = 0;
    return 0;
}

static const mimi_plugin_t echo_tool_plugin = {
    .meta = {
        .name = "echo",
        .version = "1.0.0",
        .description = "Echo input back (useful for testing)",
        .author = "MimiClaw Project"
    },
    .init = echo_tool_init,
    .exec = echo_tool_exec,
    .destroy = echo_tool_destroy
};

int mimi_tools_register_echo(mimi_tool_registry_t *reg)
{
    echo_tool_ctx_t *ctx = (echo_tool_ctx_t*)calloc(1, sizeof(echo_tool_ctx_t));
    if (ctx == NULL) {
        return MIMI_TOOLS_ERR_NO_MEMORY;
    }
    
    return mimi_registry_register_tool(reg, &echo_tool_plugin, ctx);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* mimi_tools_version(void)
{
    return MIMI_TOOLS_VERSION;
}

const char* mimi_tools_strerror(int error)
{
    switch (error) {
        case MIMI_TOOLS_OK:
            return "Success";
        case MIMI_TOOLS_ERR_INVALID_ARG:
            return "Invalid argument";
        case MIMI_TOOLS_ERR_NO_MEMORY:
            return "Memory allocation failed";
        case MIMI_TOOLS_ERR_NOT_FOUND:
            return "Tool not found";
        case MIMI_TOOLS_ERR_EXISTS:
            return "Tool already exists";
        case MIMI_TOOLS_ERR_LOAD_FAILED:
            return "Plugin load failed";
        case MIMI_TOOLS_ERR_EXEC_FAILED:
            return "Tool execution failed";
        case MIMI_TOOLS_ERR_INIT_FAILED:
            return "Tool initialization failed";
        case MIMI_TOOLS_ERR_UNLOAD_FAILED:
            return "Plugin unload failed";
        default:
            return "Unknown error";
    }
}
