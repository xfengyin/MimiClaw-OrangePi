/*
 * Telegram Bot Gateway Header
 */

#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <stdbool.h>
#include <curl/curl.h>
#include "../core/config.h"
#include "../memory/memory_store.h"

#define TG_API_BASE "https://api.telegram.org/bot"
#define TG_MAX_MESSAGE_LENGTH 4096
#define TG_MAX_USERNAME_LENGTH 128

typedef struct {
    long update_id;
    long message_id;
    long chat_id;
    char username[TG_MAX_USERNAME_LENGTH];
    char text[TG_MAX_MESSAGE_LENGTH];
    long date;
} telegram_message_t;

typedef struct {
    char bot_token[256];
    char api_url[512];
    CURL *curl;
    long last_update_id;
    bool running;
    app_config_t *config;
    memory_store_t *memory;
} telegram_bot_t;

// Function prototypes
int telegram_bot_init(telegram_bot_t *bot, const char *bot_token, app_config_t *config, memory_store_t *memory);
void telegram_bot_close(telegram_bot_t *bot);

// Bot operations
int telegram_bot_start(telegram_bot_t *bot);
void telegram_bot_stop(telegram_bot_t *bot);
int telegram_bot_send_message(telegram_bot_t *bot, long chat_id, const char *text);
int telegram_bot_send_markdown(telegram_bot_t *bot, long chat_id, const char *text);

// Message handling
int telegram_bot_get_updates(telegram_bot_t *bot, telegram_message_t **messages, int *count);
void telegram_bot_process_message(telegram_bot_t *bot, telegram_message_t *message);
void telegram_bot_free_messages(telegram_message_t *messages, int count);

// Commands
void telegram_bot_handle_start(telegram_bot_t *bot, telegram_message_t *message);
void telegram_bot_handle_help(telegram_bot_t *bot, telegram_message_t *message);
void telegram_bot_handle_chat(telegram_bot_t *bot, telegram_message_t *message);

#endif // TELEGRAM_BOT_H