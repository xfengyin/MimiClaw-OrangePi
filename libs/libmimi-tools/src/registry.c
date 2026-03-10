/**
 * @file registry.c
 * @brief MimiClaw Tools Library - Registry Core Implementation
 * @version 2.0.0
 */

#define _GNU_SOURCE

#include "mimi_tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
