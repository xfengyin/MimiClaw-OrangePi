/*
 * AI Agent Loop Header
 */

#ifndef AGENT_LOOP_H
#define AGENT_LOOP_H

#include "../core/config.h"
#include "../memory/memory_store.h"

#define MAX_RESPONSE_LENGTH 4096
#define MAX_CONTEXT_LENGTH 8192
#define MAX_TOOLS 10

// Tool function typedef
typedef int (*tool_function_t)(const char *params, char *result, size_t result_size);

// Tool definition
typedef struct {
    char name[64];
    char description[256];
    tool_function_t function;
} tool_t;

// Agent context
typedef struct {
    app_config_t *config;
    memory_store_t *memory;
    tool_t tools[MAX_TOOLS];
    int tool_count;
} agent_context_t;

// Function prototypes
int agent_init(agent_context_t *ctx, app_config_t *config, memory_store_t *memory);
void agent_close(agent_context_t *ctx);

// Tool registration
int agent_register_tool(agent_context_t *ctx, const char *name, 
                        const char *description, tool_function_t func);

// Message processing
int agent_process_message(const char *message, const char *session_id, 
                          const char *user_id, app_config_t *config, 
                          memory_store_t *memory, char *response, size_t response_size);

// LLM interaction
int agent_call_llm(const char *prompt, const char *system_prompt, 
                   const char *api_key, const char *model,
                   char *response, size_t response_size);

// Context building
int agent_build_context(const char *session_id, memory_store_t *memory,
                        char *context, size_t context_size);

#endif // AGENT_LOOP_H