/*
 * Tool Registry Header
 */

#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include "../agent/agent_loop.h"

// Tool registration
int tools_register_all(agent_context_t *ctx);

// Individual tools
int tool_web_search(const char *params, char *result, size_t result_size);
int tool_get_time(const char *params, char *result, size_t result_size);
int tool_read_file(const char *params, char *result, size_t result_size);
int tool_write_file(const char *params, char *result, size_t result_size);

#endif // TOOL_REGISTRY_H