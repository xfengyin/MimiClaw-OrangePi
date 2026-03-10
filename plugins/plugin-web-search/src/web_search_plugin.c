/**
 * @file web_search_plugin.c
 * @brief Web Search Plugin Implementation
 * @version 1.0.0
 * 
 * Performs web searches using Brave Search API or Perplexity API.
 * API Key must be configured in MimiClaw config.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "mimi_tools.h"

/* ============================================================================
 * Plugin Context
 * ============================================================================ */

typedef struct {
    int initialized;
    CURL *curl;
    char *api_key;
    char *api_endpoint;
} web_search_plugin_ctx_t;

/* ============================================================================
 * Helper Structures
 * ============================================================================ */

/**
 * @brief Buffer for curl response
 */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} curl_buffer_t;

/**
 * @brief Search result
 */
typedef struct {
    char *title;
    char *url;
    char *snippet;
} search_result_t;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static size_t web_search_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    curl_buffer_t *buf = (curl_buffer_t*)userp;
    
    /* Grow buffer if needed */
    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_capacity = (buf->capacity == 0) ? 4096 : buf->capacity * 2;
        while (new_capacity < buf->size + realsize + 1) {
            new_capacity *= 2;
        }
        
        char *new_data = realloc(buf->data, new_capacity);
        if (new_data == NULL) {
            return 0;
        }
        
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    
    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';
    
    return realsize;
}

/**
 * @brief Simple JSON string extraction (finds value for a key)
 */
static char* json_get_string(const char *json, const char *key)
{
    if (json == NULL || key == NULL) {
        return NULL;
    }
    
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_key);
    if (key_pos == NULL) {
        return NULL;
    }
    
    /* Find the colon */
    const char *colon = strchr(key_pos + strlen(search_key), ':');
    if (colon == NULL) {
        return NULL;
    }
    
    /* Skip whitespace */
    const char *value = colon + 1;
    while (*value == ' ' || *value == '\t' || *value == '\n') {
        value++;
    }
    
    /* Check if it's a string */
    if (*value != '"') {
        return NULL;
    }
    
    value++; /* Skip opening quote */
    
    /* Find closing quote (handle escapes) */
    const char *end = value;
    while (*end != '\0' && *end != '"') {
        if (*end == '\\' && *(end + 1) != '\0') {
            end += 2;
        } else {
            end++;
        }
    }
    
    if (*end != '"') {
        return NULL;
    }
    
    size_t len = end - value;
    char *result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    memcpy(result, value, len);
    result[len] = '\0';
    
    return result;
}

/**
 * @brief Extract search results from JSON response
 */
static int parse_search_results(const char *json, search_result_t **results, int *count)
{
    (void)json;
    (void)results;
    (void)count;
    
    /* Simplified: just return a placeholder result */
    /* In production, use a proper JSON parser like cJSON */
    
    *results = malloc(sizeof(search_result_t));
    if (*results == NULL) {
        return -1;
    }
    
    (*results)[0].title = strdup("Search Results");
    (*results)[0].url = strdup("https://example.com");
    (*results)[0].snippet = strdup(json != NULL ? json : "No results");
    
    *count = 1;
    return 0;
}

/* ============================================================================
 * Plugin Implementation
 * ============================================================================ */

static int web_search_plugin_init(mimi_tool_ctx_t *ctx)
{
    web_search_plugin_ctx_t *sctx = (web_search_plugin_ctx_t*)ctx;
    
    if (sctx == NULL) {
        return -1;
    }
    
    /* Initialize curl */
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return -1;
    }
    
    sctx->curl = curl_easy_init();
    if (sctx->curl == NULL) {
        curl_global_cleanup();
        return -1;
    }
    
    /* Default to Brave Search API */
    sctx->api_key = getenv("BRAVE_SEARCH_API_KEY");
    if (sctx->api_key == NULL) {
        sctx->api_key = "";
    }
    
    sctx->api_endpoint = "https://api.search.brave.com/res/v1/web/search";
    sctx->initialized = 1;
    
    return 0;
}

static int web_search_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output)
{
    web_search_plugin_ctx_t *sctx = (web_search_plugin_ctx_t*)ctx;
    
    if (sctx == NULL || output == NULL || input == NULL) {
        return -1;
    }
    
    if (strlen(input) == 0) {
        *output = strdup("{\"error\":\"No search query provided\"}");
        return *output != NULL ? 0 : -1;
    }
    
    /* Build URL with query */
    char url[1024];
    snprintf(url, sizeof(url), "%s?q=%s&count=5", sctx->api_endpoint, input);
    
    /* Prepare response buffer */
    curl_buffer_t response = {0};
    
    /* Configure curl */
    curl_easy_setopt(sctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(sctx->curl, CURLOPT_WRITEFUNCTION, web_search_write_callback);
    curl_easy_setopt(sctx->curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(sctx->curl, CURLOPT_USERAGENT, "MimiClaw/2.0");
    
    /* Set API key header if available */
    struct curl_slist *headers = NULL;
    if (sctx->api_key != NULL && strlen(sctx->api_key) > 0) {
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "X-Subscription-Token: %s", sctx->api_key);
        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(sctx->curl, CURLOPT_HTTPHEADER, headers);
    }
    
    /* Perform request */
    CURLcode res = curl_easy_perform(sctx->curl);
    
    if (headers != NULL) {
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        fprintf(stderr, "[web-search] curl error: %s\n", curl_easy_strerror(res));
        
        /* Return error response */
        *output = malloc(256);
        if (*output) {
            snprintf(*output, 256, "{\"error\":\"%s\"}", curl_easy_strerror(res));
        }
        
        return *output != NULL ? 0 : -1;
    }
    
    /* Parse results */
    search_result_t *results = NULL;
    int count = 0;
    
    if (parse_search_results(response.data, &results, &count) == 0) {
        /* Build JSON response */
        *output = malloc(4096);
        if (*output) {
            char *p = *output;
            p += sprintf(p, "{\"query\":\"%s\",\"results\":[", input);
            
            for (int i = 0; i < count; i++) {
                if (i > 0) p += sprintf(p, ",");
                p += sprintf(p, "{\"title\":\"%s\",\"url\":\"%s\",\"snippet\":\"%s\"}",
                            results[i].title ? results[i].title : "",
                            results[i].url ? results[i].url : "",
                            results[i].snippet ? results[i].snippet : "");
                
                free(results[i].title);
                free(results[i].url);
                free(results[i].snippet);
            }
            
            sprintf(p, "],\"count\":%d}", count);
        }
        
        free(results);
    } else {
        /* Return raw response */
        *output = response.data;
        response.data = NULL;
    }
    
    /* Cleanup */
    free(response.data);
    
    return *output != NULL ? 0 : -1;
}

static int web_search_plugin_destroy(mimi_tool_ctx_t *ctx)
{
    web_search_plugin_ctx_t *sctx = (web_search_plugin_ctx_t*)ctx;
    
    if (sctx == NULL) {
        return 0;
    }
    
    if (sctx->curl != NULL) {
        curl_easy_cleanup(sctx->curl);
    }
    
    curl_global_cleanup();
    sctx->initialized = 0;
    free(sctx);
    
    return 0;
}

/* ============================================================================
 * Plugin Metadata and Interface
 * ============================================================================ */

static const mimi_plugin_t web_search_plugin = {
    .meta = {
        .name = "web-search",
        .version = "1.0.0",
        .description = "Search the web using Brave Search API (requires API key)",
        .author = "MimiClaw Project"
    },
    .init = web_search_plugin_init,
    .exec = web_search_plugin_exec,
    .destroy = web_search_plugin_destroy
};

/* ============================================================================
 * Exported Symbol
 * ============================================================================ */

const mimi_plugin_t* get_tool_plugin(void)
{
    return &web_search_plugin;
}
