/**
 * @file example_gateway.c
 * @brief MimiClaw Gateway 使用示例
 * 
 * 演示如何启动网关服务
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gateway.h"

/* 外部依赖 */
extern telegram_bot_t* telegram_init(const char *token, 
                                       void (*handler)(const char*, const char*));
extern int telegram_start(telegram_bot_t *bot);
extern int telegram_send_message(const char *token, const char *chat_id, const char *text);

extern websocket_server_t* websocket_init(int port, 
                                            void (*handler)(const char*, const char*, char**));
extern int websocket_start(websocket_server_t *server);

/* WebSocket 消息处理回调 */
static void ws_message_handler(const char *session_id, const char *text, char **response) {
    printf("[WebSocket] Session: %s, Message: %s\n", session_id, text);
    
    /* 这里调用核心处理 */
    if (gateway_handle_chat(session_id, text, response) == 0) {
        printf("[WebSocket] Response: %s\n", *response ? *response : "(empty)");
    }
}

/* Telegram 消息处理回调 */
static void tg_message_handler(const char *chat_id, const char *text) {
    printf("[Telegram] Chat: %s, Message: %s\n", chat_id, text);
    
    char *response = NULL;
    char session_id[64];
    snprintf(session_id, sizeof(session_id), "tg_%s", chat_id);
    
    if (gateway_handle_chat(session_id, text, &response) == 0 && response) {
        printf("[Telegram] Response: %s\n", response);
        /* TODO: 添加 bot token 后发送 */
        free(response);
    }
}

int main(int argc, char *argv[]) {
    /* 配置 */
    gateway_config_t config = {
        .bind_addr = "0.0.0.0",
        .port = 8080,
        .api_key = getenv("MIMI_API_KEY"),
        .model = "gpt-3.5-turbo",
        .max_sessions = 100
    };
    
    /* 检查 API Key */
    if (!config.api_key) {
        fprintf(stderr, "Error: MIMI_API_KEY not set\n");
        return 1;
    }
    
    /* 初始化网关 */
    printf("[*] Initializing MimiClaw Gateway...\n");
    if (gateway_init(&config) != 0) {
        fprintf(stderr, "Failed to initialize gateway\n");
        return 1;
    }
    
    /* 初始化 Telegram (可选) */
    const char *tg_token = getenv("MIMI_TELEGRAM_BOT_TOKEN");
    telegram_bot_t *tg_bot = NULL;
    if (tg_token) {
        printf("[*] Initializing Telegram bot...\n");
        tg_bot = telegram_init(tg_token, tg_message_handler);
        if (tg_bot) {
            telegram_start(tg_bot);
        }
    }
    
    /* 初始化 WebSocket 服务器 */
    printf("[*] Starting WebSocket server on port 8081...\n");
    websocket_server_t *ws_server = websocket_init(8081, ws_message_handler);
    if (ws_server) {
        websocket_start(ws_server);
    }
    
    /* 运行 CLI 模式 */
    printf("\n[*] Gateway ready!\n");
    printf("    CLI:     127.0.0.1:8080\n");
    printf("    WebSocket: ws://127.0.0.1:8081\n");
    printf("    Telegram: %s\n\n", tg_token ? "Enabled" : "Disabled");
    
    gateway_run();
    
    /* 清理 */
    printf("\n[*] Cleaning up...\n");
    if (ws_server) {
        websocket_stop(ws_server);
    }
    if (tg_bot) {
        telegram_stop(tg_bot);
    }
    gateway_cleanup();
    
    return 0;
}