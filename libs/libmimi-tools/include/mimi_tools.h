/**
 * @file mimi_tools.h
 * @brief MimiClaw Tools Library - Tool Registry & Plugin System
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 */

#ifndef MIMI_TOOLS_H
#define MIMI_TOOLS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tools library version string
 */
#define MIMI_TOOLS_VERSION "2.0.0"

/**
 * @brief Maximum tool name length
 */
#define MIMI_TOOLS_MAX_NAME_LEN 128

/**
 * @brief Maximum description length
 */
#define MIMI_TOOLS_MAX_DESC_LEN 512

/**
 * @brief Maximum path length
 */
#define MIMI_TOOLS_MAX_PATH_LEN 512

/**
 * @brief Error codes
 */
typedef enum {
    MIMI_TOOLS_OK = 0,                /**< Success */
    MIMI_TOOLS_ERR_INVALID_ARG = -1,       /**< Invalid argument */
    MIMI_TOOLS_ERR_NO_MEMORY = -2,         /**< Memory allocation failed */
    MIMI_TOOLS_ERR_NOT_FOUND = -3,         /**< Tool not found */
    MIMI_TOOLS_ERR_EXISTS = -4,            /**< Tool already exists */
    MIMI_TOOLS_ERR_LOAD_FAILED = -5,       /**< Plugin load failed */
    MIMI_TOOLS_ERR_EXEC_FAILED = -6,       /**< Tool execution failed */
    MIMI_TOOLS_ERR_INIT_FAILED = -7,       /**< Tool initialization failed */
    MIMI_TOOLS_ERR_UNLOAD_FAILED = -8      /**< Plugin unload failed */
} mimi_tools_error_t;

/**
 * @brief Tool execution context (opaque)
 */
typedef void mimi_tool_ctx_t;

/**
 * @brief Plugin metadata - all plugins must provide this
 */
typedef struct {
    const char *name;        /**< Plugin/tool name */
    const char *version;     /**< Plugin version (e.g., "1.0.0") */
    const char *description; /**< Plugin description */
    const char *author;      /**< Plugin author */
} mimi_plugin_meta_t;

/**
 * @brief Tool plugin interface
 * 
 * All tools must implement this interface.
 * For dynamic plugins, this structure is exported from the .so file.
 */
typedef struct {
    mimi_plugin_meta_t meta; /**< Plugin metadata */
    
    /**
     * @brief Initialize the tool
     * @param ctx Tool context (created by registry)
     * @return 0 on success, negative on failure
     */
    int (*init)(mimi_tool_ctx_t *ctx);
    
    /**
     * @brief Execute the tool
     * @param ctx Tool context
     * @param input Input string (JSON or plain text)
     * @param output Output string (allocated by tool, freed by caller)
     * @return 0 on success, negative on failure
     */
    int (*exec)(mimi_tool_ctx_t *ctx, const char *input, char **output);
    
    /**
     * @brief Cleanup the tool
     * @param ctx Tool context
     * @return 0 on success
     */
    int (*destroy)(mimi_tool_ctx_t *ctx);
} mimi_plugin_t;

/**
 * @brief Opaque tool registry structure
 */
typedef struct mimi_tool_registry mimi_tool_registry_t;

/**
 * @brief Tool information structure
 */
typedef struct {
    char *name;              /**< Tool name */
    char *description;       /**< Tool description */
    char *version;           /**< Plugin version */
    char *author;            /**< Plugin author */
    int is_builtin;          /**< 1 if built-in, 0 if loaded from plugin */
    char *plugin_path;       /**< Path to plugin .so file (if applicable) */
} mimi_tool_info_t;

/* ============================================================================
 * Registry Lifecycle
 * ============================================================================ */

/**
 * @brief Create a tool registry
 * 
 * @return Registry handle, or NULL on failure
 * 
 * @note Memory: Caller must call mimi_registry_destroy() when done
 */
mimi_tool_registry_t* mimi_registry_create(void);

/**
 * @brief Destroy a tool registry
 * 
 * Unloads all plugins and frees all resources.
 * 
 * @param reg Registry to destroy (can be NULL)
 */
void mimi_registry_destroy(mimi_tool_registry_t *reg);

/* ============================================================================
 * Built-in Tool Registration
 * ============================================================================ */

/**
 * @brief Register a built-in tool
 * 
 * @param reg Registry
 * @param plugin Tool plugin structure
 * @param ctx Tool context (created by caller)
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_registry_register_tool(mimi_tool_registry_t *reg,
                                const mimi_plugin_t *plugin,
                                mimi_tool_ctx_t *ctx);

/**
 * @brief Unregister a tool
 * 
 * @param reg Registry
 * @param name Tool name to unregister
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_registry_unregister_tool(mimi_tool_registry_t *reg, const char *name);

/* ============================================================================
 * Plugin Loading (Dynamic Libraries)
 * ============================================================================ */

/**
 * @brief Load a tool from a shared library plugin
 * 
 * The plugin must export a symbol: const mimi_plugin_t* get_tool_plugin(void)
 * 
 * @param reg Registry
 * @param so_path Path to .so file
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_registry_load_plugin(mimi_tool_registry_t *reg, const char *so_path);

/**
 * @brief Unload a plugin
 * 
 * @param reg Registry
 * @param name Tool name (used to identify the plugin)
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_registry_unload_plugin(mimi_tool_registry_t *reg, const char *name);

/**
 * @brief Get a loaded plugin by name
 * 
 * @param reg Registry
 * @param name Tool name
 * @return Plugin structure, or NULL if not found
 */
const mimi_plugin_t* mimi_registry_get_plugin(mimi_tool_registry_t *reg, const char *name);

/**
 * @brief Load all plugins from a directory
 * 
 * Scans directory for .so files and loads each as a plugin.
 * 
 * @param reg Registry
 * @param dir_path Path to plugins directory
 * @return Number of plugins loaded, or negative on failure
 */
int mimi_registry_load_plugins_dir(mimi_tool_registry_t *reg, const char *dir_path);

/* ============================================================================
 * Tool Execution
 * ============================================================================ */

/**
 * @brief Execute a tool
 * 
 * @param reg Registry
 * @param tool_name Name of the tool to execute
 * @param input Input string
 * @param output Output string (allocated, must be freed)
 * @return MIMI_TOOLS_OK on success, error code on failure
 * 
 * @note Memory: Caller must free *output when done
 */
int mimi_registry_exec(mimi_tool_registry_t *reg, const char *tool_name,
                       const char *input, char **output);

/**
 * @brief Check if a tool exists
 * 
 * @param reg Registry
 * @param name Tool name
 * @return 1 if exists, 0 if not
 */
int mimi_registry_has_tool(mimi_tool_registry_t *reg, const char *name);

/**
 * @brief Get tool information
 * 
 * @param reg Registry
 * @param name Tool name
 * @return Tool info structure (allocated, must be freed), or NULL on failure
 */
mimi_tool_info_t* mimi_registry_get_tool_info(mimi_tool_registry_t *reg, const char *name);

/**
 * @brief Free tool info structure
 * 
 * @param info Info structure to free
 */
void mimi_tool_info_free(mimi_tool_info_t *info);

/* ============================================================================
 * Tool Listing
 * ============================================================================ */

/**
 * @brief List all registered tools
 * 
 * @param reg Registry
 * @param tools Pointer to receive array of tool info
 * @param count Pointer to receive number of tools
 * @return MIMI_TOOLS_OK on success, error code on failure
 * 
 * @note Memory: Caller must free each tool info and the tools array
 */
int mimi_registry_list(mimi_tool_registry_t *reg, mimi_tool_info_t ***tools, int *count);

/* ============================================================================
 * Built-in Tools
 * ============================================================================ */

/**
 * @brief Register the built-in time tool
 * 
 * Returns current date/time in various formats.
 * 
 * @param reg Registry
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_tools_register_time(mimi_tool_registry_t *reg);

/**
 * @brief Register the built-in echo tool
 * 
 * Echoes input back (useful for testing).
 * 
 * @param reg Registry
 * @return MIMI_TOOLS_OK on success, error code on failure
 */
int mimi_tools_register_echo(mimi_tool_registry_t *reg);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get the library version string
 * 
 * @return Version string (statically allocated, do not free)
 */
const char* mimi_tools_version(void);

/**
 * @brief Get error message for an error code
 * 
 * @param error Error code
 * @return Error message string (statically allocated, do not free)
 */
const char* mimi_tools_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_TOOLS_H */
