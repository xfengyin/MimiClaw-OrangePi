/*
 * Telegram Bot Gateway Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <json-c/json.h>
#include "telegram_bot.h"
#include "../core/logger.h"
#include "../agent/agent_loop.h"

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

int telegram_bot_init(telegram_bot_t *bot, const char *bot_token, app_config_t *config, memory_store_t *memory) {
    if (!bot || !bot_token) return -1;
    
    memset(bot, 0, sizeof(telegram_bot_t));
    strncpy(bot->bot_token, bot_token, sizeof(bot->bot_token) - 1);
    snprintf(bot->api_url, sizeof(bot->api_url), "%s%s", TG_API_BASE, bot_token);
    
    bot->config = config;
    bot->memory = memory;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    bot->curl = curl_easy_init();
    if (!bot->curl) {
        LOG_ERROR("Failed to initialize CURL");
        return -1;
    }
    
    // Set proxy if configured
    if (config && config->proxy.enabled) {
        char proxy_url[256];
        snprintf(proxy_url, sizeof(proxy_url), "%s://%s:%d", 
                 config->proxy.type, config->proxy.host, config->proxy.port);
        curl_easy_setopt(bot->curl, CURLOPT_PROXY, proxy_url);
        LOG_INFO("Using proxy: %s", proxy_url);
    }
    
    LOG_INFO("Telegram bot initialized");
    return 0;
}

void telegram_bot_close(telegram_bot_t *bot) {
    if (!bot) return;
    
    telegram_bot_stop(bot);
    
    if (bot->curl) {
        curl_easy_cleanup(bot->curl);
        bot->curl = NULL;
    }
    curl_global_cleanup();
    
    LOG_INFO("Telegram bot closed");
}

int telegram_bot_send_message(telegram_bot_t *bot, long chat_id, const char *text) {
    if (!bot || !bot->curl || !text) return -1;
    
    char url[512];
    snprintf(url, sizeof(url), "%s/sendMessage", bot->api_url);
    
    // Escape special characters in text
    char escaped_text[TG_MAX_MESSAGE_LENGTH * 2];
    int j = 0;
    for (int i = 0; text[i] && j < sizeof(escaped_text) - 1; i++) {
        if (text[i] == '"' || text[i] == '\\' || text[i] == '\n') {
            if (j < sizeof(escaped_text) - 2) {
                escaped_text[j++] = '\\';
            }
        }
        escaped_text[j++] = text[i];
    }
    escaped_text[j] = '\0';
    
    char post_fields[TG_MAX_MESSAGE_LENGTH * 2 + 256];
    snprintf(post_fields, sizeof(post_fields), 
             "chat_id=%ld&text=%s&parse_mode=Markdown", chat_id, escaped_text);
    
    response_buffer_t response = {0};
    
    curl_easy_setopt(bot->curl, CURLOPT_URL, url);
    curl_easy_setopt(bot->curl, CURLOPT_POSTFIELDS, post_fields);
    curl_easy_setopt(bot->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(bot->curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(bot->curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(bot->curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("Failed to send message: %s", curl_easy_strerror(res));
        free(response.data);
        return -1;
    }
    
    LOG_DEBUG("Message sent to chat %ld", chat_id);
    free(response.data);
    return 0;
}

int telegram_bot_get_updates(telegram_bot_t *bot, telegram_message_t **messages, int *count) {
    if (!bot || !bot->curl || !messages || !count) return -1;
    
    char url[512];
    snprintf(url, sizeof(url), "%s/getUpdates?offset=%ld&limit=10", 
             bot->api_url, bot->last_update_id + 1);
    
    response_buffer_t response = {0};
    
    curl_easy_setopt(bot->curl, CURLOPT_URL, url);
    curl_easy_setopt(bot->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(bot->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(bot->curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(bot->curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(bot->curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("Failed to get updates: %s", curl_easy_strerror(res));
        free(response.data);
        return -1;
    }
    
    // Parse JSON response
    json_object *root = json_tokener_parse(response.data);
    free(response.data);
    
    if (!root) {
        LOG_ERROR("Failed to parse response");
        return -1;
    }
    
    json_object *ok_obj;
    if (!json_object_object_get_ex(root, "ok", &ok_obj) || !json_object_get_boolean(ok_obj)) {
        LOG_ERROR("API returned error");
        json_object_put(root);
        return -1;
    }
    
    json_object *result_array;
    if (!json_object_object_get_ex(root, "result", &result_array)) {
        json_object_put(root);
        return 0;
    }
    
    int num_updates = json_object_array_length(result_array);
    if (num_updates == 0) {
        json_object_put(root);
        *messages = NULL;
        *count = 0;
        return 0;
    }
    
    *messages = calloc(num_updates, sizeof(telegram_message_t));
    if (!*messages) {
        json_object_put(root);
        return -1;
    }
    
    for (int i = 0; i < num_updates; i++) {
        json_object *update = json_object_array_get_idx(result_array, i);
        telegram_message_t *msg = &(*messages)[i];
        
        json_object *update_id_obj;
        if (json_object_object_get_ex(update, "update_id", &update_id_obj)) {
            msg->update_id = json_object_get_int64(update_id_obj);
            if (msg->update_id > bot->last_update_id) {
                bot->last_update_id = msg->update_id;
            }
        }
        
        json_object *message_obj;
        if (json_object_object_get_ex(update, "message", &message_obj)) {
            json_object *message_id_obj;
            if (json_object_object_get_ex(message_obj, "message_id", &message_id_obj)) {
                msg->message_id = json_object_get_int64(message_id_obj);
            }
            
            json_object *date_obj;
            if (json_object_object_get_ex(message_obj, "date", &date_obj)) {
                msg->date = json_object_get_int64(date_obj);
            }
            
            json_object *chat_obj;
            if (json_object_object_get_ex(message_obj, "chat", &chat_obj)) {
                json_object *chat_id_obj;
                if (json_object_object_get_ex(chat_obj, "id", &chat_id_obj)) {
                    msg->chat_id = json_object_get_int64(chat_id_obj);
                }
            }
            
            json_object *from_obj;
            if (json_object_object_get_ex(message_obj, "from", &from_obj)) {
                json_object *username_obj;
                if (json_object_object_get_ex(from_obj, "username", &username_obj)) {
                    strncpy(msg->username, json_object_get_string(username_obj), 
                           TG_MAX_USERNAME_LENGTH - 1);
                }
            }
            
            json_object *text_obj;
            if (json_object_object_get_ex(message_obj, "text", &text_obj)) {
                strncpy(msg->text, json_object_get_string(text_obj), 
                       TG_MAX_MESSAGE_LENGTH - 1);
            }
        }
    }
    
    *count = num_updates;
    json_object_put(root);
    return 0;
}

void telegram_bot_process_message(telegram_bot_t *bot, telegram_message_t *message) {
    if (!bot || !message) return;
    
    LOG_INFO("Message from @%s: %s", message->username, message->text);
    
    // Handle commands
    if (message->text[0] == '/') {
        if (strncmp(message->text, "/start", 6) == 0) {
            telegram_bot_handle_start(bot, message);
        } else if (strncmp(message->text, "/help", 5) == 0) {
            telegram_bot_handle_help(bot, message);
        } else {
            telegram_bot_send_message(bot, message->chat_id, 
                "Unknown command. Use /help for available commands.");
        }
    } else {
        // Regular chat message - process through AI agent
        telegram_bot_handle_chat(bot, message);
    }
}

void telegram_bot_handle_start(telegram_bot_t *bot, telegram_message_t *message) {
    const char *welcome = 
        "🤖 *Welcome to MimiClaw-OrangePi!*\n\n"
        "I'm your AI assistant running on OrangePi Zero3.\n\n"
        "*Commands:*\n"
        "/start - Show this message\n"
        "/help - Show help\n\n"
        "Just send me a message and I'll respond!";
    
    telegram_bot_send_message(bot, message->chat_id, welcome);
}

void telegram_bot_handle_help(telegram_bot_t *bot, telegram_message_t *message) {
    const char *help = 
        "*MimiClaw-OrangePi Help*\n\n"
        "I'm an AI assistant powered by Claude.\n\n"
        "*Features:*\n"
        "• Natural conversation\n"
        "• Long-term memory\n"
        "• Web search capability\n"
        "• File operations\n\n"
        "*Tips:*\n"
        "• Be specific in your questions\n"
        "• I remember our conversations\n"
        "• Ask me anything!";
    
    telegram_bot_send_message(bot, message->chat_id, help);
}

void telegram_bot_handle_chat(telegram_bot_t *bot, telegram_message_t *message) {
    // Create or get session
    char session_id[64];
    snprintf(session_id, sizeof(session_id), "tg_%ld", message->chat_id);
    
    // Process through AI agent
    char response[TG_MAX_MESSAGE_LENGTH];
    
    // Call agent to process message
    int result = agent_process_message(message->text, session_id, message->username,
                                       bot->config, bot->memory, 
                                       response, sizeof(response));
    
    if (result == 0) {
        telegram_bot_send_message(bot, message->chat_id, response);
    } else {
        telegram_bot_send_message(bot, message->chat_id, 
            "Sorry, I encountered an error processing your message. Please try again.");
    }
}

void telegram_bot_free_messages(telegram_message_t *messages, int count) {
    if (messages) {
        free(messages);
    }
    (void)count;
}

static void* telegram_bot_poll_thread(void *arg) {
    telegram_bot_t *bot = (telegram_bot_t *)arg;
    
    LOG_INFO("Telegram bot polling started");
    
    while (bot->running) {
        telegram_message_t *messages = NULL;
        int count = 0;
        
        if (telegram_bot_get_updates(bot, &messages, &count) == 0 && count > 0) {
            for (int i = 0; i < count; i++) {
                telegram_bot_process_message(bot, &messages[i]);
            }
            telegram_bot_free_messages(messages, count);
        }
        
        // Poll every 1 second
        sleep(1);
    }
    
    LOG_INFO("Telegram bot polling stopped");
    return NULL;
}

int telegram_bot_start(telegram_bot_t *bot) {
    if (!bot || bot->running) return -1;
    
    bot->running = true;
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, telegram_bot_poll_thread, bot) != 0) {
        bot->running = false;
        LOG_ERROR("Failed to create bot polling thread");
        return -1;
    }
    
    pthread_detach(thread);
    LOG_INFO("Telegram bot started");
    return 0;
}

void telegram_bot_stop(telegram_bot_t *bot) {
    if (!bot) return;
    bot->running = false;
}