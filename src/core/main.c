/*
 * MimiClaw-OrangePi Main Entry Point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include "config.h"
#include "logger.h"
#include "../memory/memory_store.h"
#include "../gateway/telegram_bot.h"
#include "../gateway/websocket_server.h"
#include "../agent/agent_loop.h"

#define VERSION "1.0.0"
#define PROGRAM_NAME "mimiclaw-orangepi"

static volatile int running = 1;
static app_config_t g_config;
static memory_store_t g_memory;
static telegram_bot_t g_telegram_bot;
static ws_server_t g_ws_server;

void signal_handler(int sig) {
    LOG_INFO("Received signal %d, shutting down...", sig);
    running = 0;
}

void print_usage(const char *program) {
    printf("Usage: %s [OPTIONS]\n\n", program);
    printf("Options:\n");
    printf("  -c, --config FILE    Configuration file path (default: /etc/mimiclaw/config.json)\n");
    printf("  -d, --daemon         Run as daemon\n");
    printf("  -v, --verbose        Verbose output\n");
    printf("  -V, --version        Show version\n");
    printf("  -h, --help           Show this help\n");
}

void print_version() {
    printf("%s v%s\n", PROGRAM_NAME, VERSION);
    printf("MimiClaw AI Assistant for OrangePi Zero3\n");
    printf("Based on MimiClaw for ESP32-S3\n");
}

int create_directories(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
    
    return 0;
}

int main(int argc, char *argv[]) {
    const char *config_file = "/etc/mimiclaw/config.json";
    int daemon_mode = 0;
    int verbose = 0;
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"daemon", no_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "c:dvVh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'd':
                daemon_mode = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'V':
                print_version();
                return 0;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    // Initialize logger
    log_level_t log_level = verbose ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO;
    logger_init(NULL, log_level);
    
    LOG_INFO("Starting %s v%s...", PROGRAM_NAME, VERSION);
    LOG_INFO("Config file: %s", config_file);
    
    // Load configuration
    if (config_load(config_file, &g_config) != 0) {
        LOG_WARN("Using default configuration");
    }
    
    if (verbose) {
        logger_set_level(LOG_LEVEL_DEBUG);
    }
    
    // Daemon mode
    if (daemon_mode) {
        LOG_INFO("Running as daemon");
        if (daemon(0, 0) != 0) {
            LOG_ERROR("Failed to daemonize");
            return 1;
        }
    }
    
    // Create data directory
    create_directories(g_config.system.data_directory);
    create_directories(g_config.memory.database_path);
    
    // Initialize memory store
    LOG_INFO("Initializing memory store...");
    if (memory_store_init(&g_memory, g_config.memory.database_path) != 0) {
        LOG_ERROR("Failed to initialize memory store");
        logger_close();
        return 1;
    }
    
    // Initialize Telegram bot
    if (g_config.telegram.enabled && strlen(g_config.telegram.bot_token) > 0) {
        LOG_INFO("Initializing Telegram bot...");
        if (telegram_bot_init(&g_telegram_bot, g_config.telegram.bot_token, 
                              &g_config, &g_memory) == 0) {
            if (telegram_bot_start(&g_telegram_bot) != 0) {
                LOG_ERROR("Failed to start Telegram bot");
            }
        } else {
            LOG_ERROR("Failed to initialize Telegram bot");
        }
    } else {
        LOG_WARN("Telegram bot disabled or token not configured");
    }
    
    // Initialize WebSocket server
    if (g_config.gateway.websocket.enabled) {
        LOG_INFO("Initializing WebSocket server on port %d...", 
                 g_config.gateway.websocket.port);
        if (ws_server_init(&g_ws_server, g_config.gateway.websocket.port,
                          &g_config, &g_memory) == 0) {
            if (ws_server_start(&g_ws_server) != 0) {
                LOG_ERROR("Failed to start WebSocket server");
            }
        } else {
            LOG_ERROR("Failed to initialize WebSocket server");
        }
    }
    
    LOG_INFO("%s started successfully!", PROGRAM_NAME);
    LOG_INFO("Press Ctrl+C to stop");
    
    // Main loop
    while (running) {
        sleep(1);
    }
    
    // Cleanup
    LOG_INFO("Shutting down...");
    
    telegram_bot_stop(&g_telegram_bot);
    telegram_bot_close(&g_telegram_bot);
    
    ws_server_stop(&g_ws_server);
    ws_server_close(&g_ws_server);
    
    memory_store_close(&g_memory);
    
    logger_close();
    
    printf("Goodbye!\n");
    return 0;
}