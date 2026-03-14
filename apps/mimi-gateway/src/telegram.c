/**
 * @file telegram.c
 * @brief MimiClaw Gateway - Telegram Bot Support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "gateway.h"

#define TG_MAX_MESSAGE_SIZE 4096
#define TG_API_URL "https://api.telegram.org/bot%s/%s"

typedef struct {
    char bot_token[128];
    char chat_id[64];
    int64_t update_id;
    pthread_t thread;
    void (*message_handler)(const char *chat_id, const char *text);
    int running;
} telegram_bot_t;

static telegram_bot_t *g_tg_bot = NULL;

/* JSON parsing helpers */
static int tg_parse_update(const char *json, char *chat_id, char *text, int64_t *update_id) {
    /* Simple JSON parsing - extract chat_id and text */
    const char *chat_ptr = strstr(json, "\"chat\":{\"id\":");
    const char *text_ptr = strstr(json, "\"text\":\"");
    const char *update_ptr = strstr(json, "\"update_id\":");

    if (!chat_ptr || !text_ptr) return -1;

    /* Extract chat_id */
    sscanf(chat_ptr, "\"chat\":{\"id\":%s", chat_id);
    char *p = strchr(chat_id, ',');
    if (p) *p = '\0';
    if (chat_id[0] == '"') memmove(chat_id, chat_id+1, strlen(chat_id));

    /* Extract text */
    const char *tstart = text_ptr + 8;
    const char *tend = strchr(tstart, '"');
    if (tend) {
        int len = tend - tstart;
        if (len > TG_MAX_MESSAGE_SIZE - 1) len = TG_MAX_MESSAGE_SIZE - 1;
        strncpy(text, tstart, len);
        text[len] = '\0';
    }

    /* Extract update_id */
    if (update_ptr) {
        sscanf(update_ptr, "\"update_id\":%lld", (long long*)update_id);
    }

    return 0;
}

/* HTTP response callback */
static size_t tg_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **buffer = (char**)userp;
    
    *buffer = realloc(*buffer, strlen(*buffer) + realsize + 1);
    if (*buffer) {
        memcpy(*buffer + strlen(*buffer), contents, realsize);
        (*buffer)[strlen(*buffer)] = 0;
    }
    return realsize;
}

/* Send message to Telegram */
int telegram_send_message(const char *bot_token, const char *chat_id, const char *text) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    char url[256];
    char *json = NULL;
    char *response = calloc(1, 1);
    
    snprintf(url, sizeof(url), TG_API_URL, bot_token, "sendMessage");

    /* Build JSON payload */
    asprintf(&json, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, text);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tg_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_free(json);
    free(response);
    curl_easy_cleanup(curl);

    return res == CURLE_OK ? 0 : -1;
}

/* Polling thread */
static void* tg_poll_thread(void *arg) {
    telegram_bot_t *bot = (telegram_bot_t*)arg;
    CURL *curl = curl_easy_init();
    
    char url[256];
    char *response = calloc(1, 1);

    while (bot->running) {
        snprintf(url, sizeof(url), TG_API_URL, bot->bot_token, "getUpdates?timeout=60");
        
        if (bot->update_id > 0) {
            snprintf(url, sizeof(url), TG_API_URL, bot->bot_token, 
                    "getUpdates?offset=%lld&timeout=60", (long long)(bot->update_id + 1));
        }

        free(response);
        response = calloc(1, 1);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tg_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        if (curl_easy_perform(curl) == CURLE_OK && strlen(response) > 20) {
            char chat_id[64] = {0};
            char text[TG_MAX_MESSAGE_SIZE] = {0};
            int64_t update_id = 0;

            if (tg_parse_update(response, chat_id, text, &update_id) == 0) {
                if (update_id > bot->update_id) {
                    bot->update_id = update_id;
                }
                if (bot->message_handler && strlen(text) > 0) {
                    bot->message_handler(chat_id, text);
                }
            }
        }

        /* Rate limiting - sleep between polls */
        sleep(1);
    }

    free(response);
    curl_easy_cleanup(curl);
    return NULL;
}

/* Initialize Telegram bot */
telegram_bot_t* telegram_init(const char *bot_token, 
                               void (*handler)(const char*, const char*)) {
    g_tg_bot = calloc(1, sizeof(telegram_bot_t));
    if (!g_tg_bot) return NULL;

    strncpy(g_tg_bot->bot_token, bot_token, sizeof(g_tg_bot->bot_token) - 1);
    g_tg_bot->message_handler = handler;
    g_tg_bot->running = 0;

    return g_tg_bot;
}

/* Start polling */
int telegram_start(telegram_bot_t *bot) {
    if (!bot) return -1;
    bot->running = 1;
    return pthread_create(&bot->thread, NULL, tg_poll_thread, bot);
}

/* Stop polling */
void telegram_stop(telegram_bot_t *bot) {
    if (!bot) return;
    bot->running = 0;
    pthread_join(bot->thread, NULL);
}

/* Cleanup */
void telegram_cleanup(telegram_bot_t *bot) {
    if (bot) free(bot);
    g_tg_bot = NULL;
}