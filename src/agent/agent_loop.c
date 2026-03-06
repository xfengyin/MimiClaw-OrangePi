/*
 * AI Agent Loop Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "agent_loop.h"
#include "../core/logger.h"
#include "../tools/tool_registry.h"

// HTTP response buffer
typedef struct {
    char *data;
    size_t size;
} response_buffer_t;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    response_buffer_t *mem = (response_buffer_t *)userp;
    
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    
    return realsize;
}

int agent_init(agent_context_t *ctx, app_config_t *config, memory_store_t *memory) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(agent_context_t));
    ctx->config = config;
    ctx->memory = memory;
    
    // Register built-in tools
    tools_register_all(ctx);
    
    LOG_INFO("Agent initialized with %d tools", ctx->tool_count);
    return 0;
}

void agent_close(agent_context_t *ctx) {
    if (!ctx) return;
    // Cleanup if needed
    LOG_INFO("Agent closed");
}

int agent_register_tool(agent_context_t *ctx, const char *name, 
                        const char *description, tool_function_t func) {
    if (!ctx || !name || !func || ctx->tool_count >= MAX_TOOLS) return -1;
    
    tool_t *tool = &ctx->tools[ctx->tool_count];
    strncpy(tool->name, name, sizeof(tool->name) - 1);
    strncpy(tool->description, description, sizeof(tool->description) - 1);
    tool->function = func;
    
    ctx->tool_count++;
    LOG_DEBUG("Tool registered: %s", name);
    return 0;
}

int agent_build_context(const char *session_id, memory_store_t *memory,
                        char *context, size_t context_size) {
    if (!session_id || !memory || !context || context_size == 0) return -1;
    
    context[0] = '\0';
    
    // Get session history
    char *history = NULL;
    int message_count = 0;
    
    if (session_get_history(memory, session_id, &history, &message_count) == 0 && history) {
        strncat(context, "Conversation history:\n", context_size - strlen(context) - 1);
        strncat(context, history, context_size - strlen(context) - 1);
        strncat(context, "\n\n", context_size - strlen(context) - 1);
        free(history);
    }
    
    // Get long-term memories
    memory_entry_t *memories = NULL;
    int memory_count = 0;
    
    if (memory_get_by_type(memory, MEMORY_TYPE_LONG_TERM, &memories, &memory_count) == 0) {
        if (memory_count > 0) {
            strncat(context, "Relevant memories:\n", context_size - strlen(context) - 1);
            for (int i = 0; i < memory_count && i < 5; i++) {
                strncat(context, "- ", context_size - strlen(context) - 1);
                strncat(context, memories[i].content, context_size - strlen(context) - 1);
                strncat(context, "\n", context_size - strlen(context) - 1);
            }
            strncat(context, "\n", context_size - strlen(context) - 1);
        }
        memory_free_entries(memories, memory_count);
    }
    
    return 0;
}

int agent_call_llm(const char *prompt, const char *system_prompt, 
                   const char *api_key, const char *model,
                   char *response, size_t response_size) {
    if (!prompt || !api_key || !response || response_size == 0) return -1;
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return -1;
    }
    
    // Build JSON request
    json_object *root = json_object_new_object();
    json_object_object_add(root, "model", json_object_new_string(model));
    json_object_object_add(root, "max_tokens", json_object_new_int(4096));
    json_object_object_add(root, "temperature", json_object_new_double(0.7));
    
    // Messages array
    json_object *messages = json_object_new_array();
    
    // System message
    if (system_prompt) {
        json_object *sys_msg = json_object_new_object();
        json_object_object_add(sys_msg, "role", json_object_new_string("system"));
        json_object_object_add(sys_msg, "content", json_object_new_string(system_prompt));
        json_object_array_add(messages, sys_msg);
    }
    
    // User message
    json_object *user_msg = json_object_new_object();
    json_object_object_add(user_msg, "role", json_object_new_string("user"));
    json_object_object_add(user_msg, "content", json_object_new_string(prompt));
    json_object_array_add(messages, user_msg);
    
    json_object_object_add(root, "messages", messages);
    
    const char *json_str = json_object_to_json_string(root);
    
    // Set up headers
    struct curl_slist *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    
    response_buffer_t buffer = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    
    LOG_DEBUG("Calling LLM API...");
    CURLcode res = curl_easy_perform(curl);
    
    json_object_put(root);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        LOG_ERROR("LLM API request failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }
    
    // Parse response
    json_object *resp_root = json_tokener_parse(buffer.data);
    free(buffer.data);
    
    if (!resp_root) {
        LOG_ERROR("Failed to parse LLM response");
        curl_easy_cleanup(curl);
        return -1;
    }
    
    // Extract content
    json_object *content_array;
    if (json_object_object_get_ex(resp_root, "content", &content_array)) {
        int content_len = json_object_array_length(content_array);
        response[0] = '\0';
        
        for (int i = 0; i < content_len; i++) {
            json_object *content_item = json_object_array_get_idx(content_array, i);
            json_object *type_obj;
            if (json_object_object_get_ex(content_item, "type", &type_obj)) {
                const char *type = json_object_get_string(type_obj);
                if (strcmp(type, "text") == 0) {
                    json_object *text_obj;
                    if (json_object_object_get_ex(content_item, "text", &text_obj)) {
                        const char *text = json_object_get_string(text_obj);
                        strncat(response, text, response_size - strlen(response) - 1);
                    }
                }
            }
        }
    } else {
        // Check for error
        json_object *error_obj;
        if (json_object_object_get_ex(resp_root, "error", &error_obj)) {
            LOG_ERROR("LLM API error: %s", json_object_to_json_string(error_obj));
            strncpy(response, "I'm sorry, I encountered an error. Please try again.", response_size - 1);
        }
    }
    
    json_object_put(resp_root);
    curl_easy_cleanup(curl);
    
    return 0;
}

int agent_process_message(const char *message, const char *session_id, 
                          const char *user_id, app_config_t *config, 
                          memory_store_t *memory, char *response, size_t response_size) {
    if (!message || !session_id || !config || !response || response_size == 0) return -1;
    
    // Ensure session exists
    session_create(memory, session_id, user_id ? user_id : "unknown");
    
    // Save user message
    session_add_message(memory, session_id, "user", message);
    
    // Build context
    char context[MAX_CONTEXT_LENGTH];
    agent_build_context(session_id, memory, context, sizeof(context));
    
    // Build prompt
    char prompt[MAX_CONTEXT_LENGTH + 1024];
    snprintf(prompt, sizeof(prompt), 
             "%s\n\nUser: %s\n\nAssistant:", context, message);
    
    // Call LLM
    int result = agent_call_llm(prompt, config->personality.system_prompt,
                                config->anthropic.api_key, config->anthropic.model,
                                response, response_size);
    
    if (result == 0) {
        // Save assistant response
        session_add_message(memory, session_id, "assistant", response);
        LOG_INFO("Agent response generated for session %s", session_id);
    } else {
        strncpy(response, "I'm sorry, I'm having trouble processing your request. Please try again later.", 
                response_size - 1);
    }
    
    return result;
}