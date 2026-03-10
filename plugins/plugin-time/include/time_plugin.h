/**
 * @file time_plugin.h
 * @brief Time Plugin - Get current date/time in various formats
 * @version 1.0.0
 */

#ifndef TIME_PLUGIN_H
#define TIME_PLUGIN_H

#include "mimi_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the plugin interface
 * 
 * This function is exported from the shared library.
 * 
 * @return Pointer to plugin interface structure
 */
const mimi_plugin_t* get_tool_plugin(void);

#ifdef __cplusplus
}
#endif

#endif /* TIME_PLUGIN_H */
