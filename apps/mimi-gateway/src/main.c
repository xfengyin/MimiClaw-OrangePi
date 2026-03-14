/**
 * @file main.c
 * @brief MimiClaw Gateway v2.0 - Multi-protocol Entry Point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <libgen.h>
#include "gateway.h"

static volatile int g_running = 1;

/* External dependencies */
extern telegram_bot_t* telegram_init(const char *token, 
                                      void (*handler)(const char*, const char*));
extern int telegram_start(telegram_bot_t *bot);
extern void telegram_stop(telegram_bot_t *bot);
extern void telegram_cleanup(telegram_bot_t *bot);
extern int telegram_send_message(const char *token, const char *chat_id, const char *text);

extern websocket_server_t* websocket_init(int port, 
                                           void (*handler)(const char*, const char*, char**));
extern int websocket_start(websocket_server_t *server);
extern void websocket_stop(websocket_server_t *server);
extern void websocket_cleanup(websocket_server_t *server);

/* Global instances */
static telegram_bot_t *g_tg_bot = NULL;
static websocket_server_t *g_ws_server = NULL;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    gateway_stop();
}

static void print_banner(void) {
    printf("============================================\n");
    printf("  MimiClaw Gateway v2.0\n");
    printf("  AI Assistant for OrangePi Zero3\n");
    printf("  Multi-Protocol: CLI + Telegram + WebSocket\n");
    printf("============================================\n");
    printf("\n");
}

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n", basename((char*)prog));
    printf("\nOptions:\n");
    printf("  -a, --addr ADDR      Bind address (default: 127.0.0.1)\n");
    printf("  -p, --port PORT      CLI/API port (default: 8080)\n");
    printf("  -w, --ws-port PORT   WebSocket port (default: 8081)\n");
    printf("  -k, --key APIKEY     OpenAI API key (required)\n");
    printf("  -m, --model MODEL    Model name (default: gpt-3.5-turbo)\n");
    printf("  -s, --sessions N     Max sessions (default: 100)\n");
    printf("  -t, --telegram TOKEN  Telegram bot token\n");
    printf("  -c, --cli            Run in CLI mode (interactive)\n");
    printf("  -d, --daemon         Run as daemon\n");
    printf("  -h, --help           Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -k YOUR_API_KEY                    # CLI mode\n", basename((char*)prog));
    printf("  %s -k YOUR_API_KEY -t TELEGRAM_TOKEN # With Telegram\n", basename((char*)prog));
    printf("  %s -k YOUR_API_KEY --daemon          # Daemon mode\n", basename((char*)prog));
    printf("\n");
}

/* CLI message handler for Telegram */
static void tg_message_handler(const char *chat_id, const char *text) {
    char *response = NULL;
    
    printf("[Telegram] Chat %s: %s\n", chat_id, text);
    
    /* Process message through gateway */
    char session_id[64];
    snprintf(session_id, sizeof(session_id), "tg_%s", chat_id);
    
    if (gateway_handle_chat(session_id, text, &response) == 0) {
        if (response) {
            printf("[Telegram] Response: %s\n", response);
            /* TODO: Send via telegram_send_message with bot token */
            free(response);
        }
    }
}

/* WebSocket message handler */
static void ws_message_handler(const char *session_id, const char *text, char **response) {
    printf("[WebSocket] Session %s: %s\n", session_id, text);
    gateway_handle_chat(session_id, text, response);
}

int main(int argc, char *argv[]) {
    gateway_config_t config = {
        .bind_addr = "127.0.0.1",
        .port = 8080,
        .api_key = NULL,
        .model = "gpt-3.5-turbo",
        .max_sessions = 100
    };
    
    int ws_port = 8081;
    char *tg_token = NULL;
    int cli_mode = 0;
    int daemon_mode = 0;

    /* Parse arguments */
    static struct option long_options[] = {
        {"addr", required_argument, 0, 'a'},
        {"port", required_argument, 0, 'p'},
        {"ws-port", required_argument, 0, 'w'},
        {"key", required_argument, 0, 'k'},
        {"model", required_argument, 0, 'm'},
        {"sessions", required_argument, 0, 's'},
        {"telegram", required_argument, 0, 't'},
        {"cli", no_argument, 0, 'c'},
        {"daemon", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:p:w:k:m:s:t:cdh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a':
                config.bind_addr = optarg;
                break;
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'w':
                ws_port = atoi(optarg);
                break;
            case 'k':
                config.api_key = optarg;
                break;
            case 'm':
                config.model = optarg;
                break;
            case 's':
                config.max_sessions = atoi(optarg);
                break;
            case 't':
                tg_token = optarg;
                break;
            case 'c':
                cli_mode = 1;
                break;
            case 'd':
                daemon_mode = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    print_banner();

    /* Validate required config */
    if (!config.api_key) {
        fprintf(stderr, "Error: API key is required (-k/--key)\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize gateway core */
    printf("[*] Initializing MimiClaw Gateway...\n");
    if (gateway_init(&config) != 0) {
        fprintf(stderr, "Error: Failed to initialize gateway\n");
        return 1;
    }

    /* Initialize Telegram bot if token provided */
    if (tg_token) {
        printf("[*] Initializing Telegram bot...\n");
        g_tg_bot = telegram_init(tg_token, tg_message_handler);
        if (g_tg_bot) {
            telegram_start(g_tg_bot);
            printf("[+] Telegram bot started\n");
        }
    }

    /* Initialize WebSocket server (always) */
    printf("[*] Starting WebSocket server on port %d...\n", ws_port);
    g_ws_server = websocket_init(ws_port, ws_message_handler);
    if (g_ws_server) {
        websocket_start(g_ws_server);
        printf("[+] WebSocket server listening on ws://%s:%d\n", 
               config.bind_addr, ws_port);
    }

    printf("[*] Gateway ready\n");
    printf("\n");

    if (cli_mode || (!tg_token && !daemon_mode)) {
        /* Run in CLI interactive mode */
        printf("Mode: Interactive CLI\n");
        printf("WebSocket: ws://%s:%d\n", config.bind_addr, ws_port);
        printf("Type 'quit' to exit, 'new' for new session\n\n");
        
        gateway_run();
    } else {
        /* Run as daemon - just wait for signals */
        printf("Mode: Daemon\n");
        printf("WebSocket: ws://%s:%d\n", config.bind_addr, ws_port);
        
        while (g_running) {
            sleep(1);
        }
    }

    /* Cleanup */
    printf("\n[*] Shutting down...\n");
    
    if (g_ws_server) {
        websocket_stop(g_ws_server);
        websocket_cleanup(g_ws_server);
    }
    
    if (g_tg_bot) {
        telegram_stop(g_tg_bot);
        telegram_cleanup(g_tg_bot);
    }
    
    gateway_cleanup();

    printf("[+] Goodbye!\n");
    return 0;
}