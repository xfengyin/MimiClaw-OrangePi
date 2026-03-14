/**
 * @file telegram.h
 * @brief Telegram Bot Support
 */

#ifndef MIMI_TELEGRAM_H
#define MIMI_TELEGRAM_H

#include <stdint.h>
#include <pthread.h>

typedef struct telegram_bot telegram_bot_t;

/**
 * Initialize Telegram bot
 * @param bot_token Bot API token from @BotFather
 * @param handler Callback for incoming messages
 * @return Bot instance or NULL on error
 */
telegram_bot_t* telegram_init(const char *bot_token,
                              void (*handler)(const char *chat_id, const char *text));

/**
 * Start polling for updates
 */
int telegram_start(telegram_bot_t *bot);

/**
 * Stop polling
 */
void telegram_stop(telegram_bot_t *bot);

/**
 * Send message to chat
 */
int telegram_send_message(const char *bot_token, const char *chat_id, const char *text);

/**
 * Cleanup and free bot
 */
void telegram_cleanup(telegram_bot_t *bot);

#endif /* MIMI_TELEGRAM_H */