/*
 * Tool Registry Implementation
 */

#include <string.h>
#include "tool_registry.h"
#include "../core/logger.h"

int tools_register_all(agent_context_t *ctx) {
    if (!ctx) return -1;
    
    // Register web search tool
    agent_register_tool(ctx, "web_search", 
        "Search the web for information. Usage: {\"query\": \"search terms\"}",
        tool_web_search);
    
    // Register time tool
    agent_register_tool(ctx, "get_time",
        "Get current date and time. Usage: {}",
        tool_get_time);
    
    // Register file read tool
    agent_register_tool(ctx, "read_file",
        "Read a file. Usage: {\"path\": \"/path/to/file\"}",
        tool_read_file);
    
    // Register file write tool
    agent_register_tool(ctx, "write_file",
        "Write to a file. Usage: {\"path\": \"/path/to/file\", \"content\": \"text\"}",
        tool_write_file);
    
    LOG_INFO("Registered %d tools", ctx->tool_count);
    return 0;
}