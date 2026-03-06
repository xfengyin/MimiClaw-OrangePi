/*
 * Web Search Tool Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "tool_registry.h"
#include "../core/logger.h"

#define BRAVE_API_BASE "https://api.search.brave.com/res/v1/web/search"
#define MAX_SEARCH_RESULTS 5

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

int tool_web_search(const char *params, char *result, size_t result_size) {
    if (!params || !result || result_size == 0) return -1;
    
    // Parse params
    json_object *params_obj = json_tokener_parse(params);
    if (!params_obj) {
        strncpy(result, "Error: Invalid parameters", result_size - 1);
        return -1;
    }
    
    json_object *query_obj;
    if (!json_object_object_get_ex(params_obj, "query", &query_obj)) {
        strncpy(result, "Error: Missing 'query' parameter", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    const char *query = json_object_get_string(query_obj);
    LOG_INFO("Web search: %s", query);
    
    // Get API key from environment
    const char *api_key = getenv("BRAVE_API_KEY");
    if (!api_key) {
        strncpy(result, "Error: BRAVE_API_KEY not set", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        strncpy(result, "Error: Failed to initialize CURL", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    // Build URL
    char url[512];
    char *encoded_query = curl_easy_escape(curl, query, 0);
    snprintf(url, sizeof(url), "%s?q=%s&count=%d", BRAVE_API_BASE, encoded_query, MAX_SEARCH_RESULTS);
    curl_free(encoded_query);
    
    // Set up headers
    struct curl_slist *headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "X-Subscription-Token: %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Accept: application/json");
    
    response_buffer_t buffer = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        snprintf(result, result_size, "Error: Search request failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        json_object_put(params_obj);
        return -1;
    }
    
    // Parse response
    json_object *resp_root = json_tokener_parse(buffer.data);
    free(buffer.data);
    curl_easy_cleanup(curl);
    
    if (!resp_root) {
        strncpy(result, "Error: Failed to parse search results", result_size - 1);
        json_object_put(params_obj);
        return -1;
    }
    
    // Build result string
    result[0] = '\0';
    strncat(result, "Search results:\n\n", result_size - strlen(result) - 1);
    
    json_object *web_results;
    if (json_object_object_get_ex(resp_root, "web", &web_results)) {
        json_object *results_array;
        if (json_object_object_get_ex(web_results, "results", &results_array)) {
            int num_results = json_object_array_length(results_array);
            
            for (int i = 0; i < num_results && i < MAX_SEARCH_RESULTS; i++) {
                json_object *item = json_object_array_get_idx(results_array, i);
                
                json_object *title_obj;
                json_object *url_obj;
                json_object *desc_obj;
                
                const char *title = "";
                const char *url = "";
                const char *description = "";
                
                if (json_object_object_get_ex(item, "title", &title_obj)) {
                    title = json_object_get_string(title_obj);
                }
                if (json_object_object_get_ex(item, "url", &url_obj)) {
                    url = json_object_get_string(url_obj);
                }
                if (json_object_object_get_ex(item, "description", &desc_obj)) {
                    description = json_object_get_string(desc_obj);
                }
                
                char entry[1024];
                snprintf(entry, sizeof(entry), "%d. %s\n   URL: %s\n   %s\n\n", 
                        i + 1, title, url, description);
                strncat(result, entry, result_size - strlen(result) - 1);
            }
        }
    }
    
    json_object_put(resp_root);
    json_object_put(params_obj);
    
    return 0;
}