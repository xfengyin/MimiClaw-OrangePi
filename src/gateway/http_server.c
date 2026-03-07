/*
 * HTTP Server Implementation - Health Check and Metrics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "http_server.h"
#include "../core/logger.h"

static void http_send_response(int client_socket, int status_code, const char *content_type, 
                               const char *body) {
    const char *status_text = (status_code == 200) ? "OK" : 
                              (status_code == 404) ? "Not Found" : "Error";
    
    char response[HTTP_MAX_BODY + 512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_code, status_text, content_type, strlen(body), body);
    
    send(client_socket, response, strlen(response), 0);
}

static void handle_health(http_server_t *server, int client_socket) {
    (void)server;
    
    const char *health = "{\"status\": \"healthy\", \"timestamp\": \"";
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    char response[256];
    snprintf(response, sizeof(response), "%s%s\"}", health, time_str);
    
    http_send_response(client_socket, 200, "application/json", response);
}

static void handle_metrics(http_server_t *server, int client_socket) {
    char buffer[8192];
    metrics_export_prometheus(server->metrics, buffer, sizeof(buffer));
    http_send_response(client_socket, 200, "text/plain; charset=utf-8", buffer);
}

static void handle_root(http_server_t *server, int client_socket) {
    (void)server;
    
    const char *html = 
        "<!DOCTYPE html>"
        "<html><head><title>MimiClaw-OrangePi</title></head>"
        "<body><h1>MimiClaw-OrangePi</h1>"
        "<p>AI Assistant for OrangePi Zero3</p>"
        "<ul>"
        "<li><a href=\"/health\">Health Check</a></li>"
        "<li><a href=\"/metrics\">Metrics</a></li>"
        "</ul></body></html>";
    
    http_send_response(client_socket, 200, "text/html", html);
}

static void* http_client_handler(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    char buffer[HTTP_MAX_BODY];
    int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received > 0) {
        buffer[received] = '\0';
        
        // Parse request
        if (strncmp(buffer, "GET /health", 11) == 0) {
            handle_health(NULL, client_socket);
        } else if (strncmp(buffer, "GET /metrics", 12) == 0) {
            handle_metrics(NULL, client_socket);
        } else if (strncmp(buffer, "GET /", 5) == 0) {
            handle_root(NULL, client_socket);
        } else {
            http_send_response(client_socket, 404, "text/plain", "Not Found");
        }
    }
    
    close(client_socket);
    return NULL;
}

static void* http_accept_loop(void *arg) {
    http_server_t *server = (http_server_t*)arg;
    
    LOG_INFO("HTTP server accept loop started on port %d", server->port);
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_socket = accept(server->server_socket, 
                                   (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            if (server->running) {
                LOG_ERROR("HTTP accept failed");
            }
            continue;
        }
        
        // Handle client in new thread
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;
        
        pthread_t thread;
        pthread_create(&thread, NULL, http_client_handler, socket_ptr);
        pthread_detach(thread);
    }
    
    return NULL;
}

int http_server_init(http_server_t *server, int port, app_config_t *config, metrics_registry_t *metrics) {
    if (!server) return -1;
    
    memset(server, 0, sizeof(http_server_t));
    server->port = port;
    server->config = config;
    server->metrics = metrics;
    
    // Create socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        LOG_ERROR("Failed to create HTTP server socket");
        return -1;
    }
    
    // Allow reuse
    int opt = 1;
    setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind HTTP server to port %d", port);
        close(server->server_socket);
        return -1;
    }
    
    // Listen
    if (listen(server->server_socket, 10) < 0) {
        LOG_ERROR("Failed to listen on HTTP socket");
        close(server->server_socket);
        return -1;
    }
    
    LOG_INFO("HTTP server initialized on port %d", port);
    return 0;
}

void http_server_close(http_server_t *server) {
    if (!server) return;
    
    http_server_stop(server);
    
    if (server->server_socket >= 0) {
        close(server->server_socket);
        server->server_socket = -1;
    }
    
    LOG_INFO("HTTP server closed");
}

int http_server_start(http_server_t *server) {
    if (!server || server->running) return -1;
    
    server->running = true;
    
    if (pthread_create(&server->thread, NULL, http_accept_loop, server) != 0) {
        server->running = false;
        LOG_ERROR("Failed to create HTTP server thread");
        return -1;
    }
    
    LOG_INFO("HTTP server started");
    return 0;
}

void http_server_stop(http_server_t *server) {
    if (!server) return;
    
    server->running = false;
    LOG_INFO("HTTP server stopped");
}