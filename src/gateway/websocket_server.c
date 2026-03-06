/*
 * WebSocket Server Implementation (Simplified)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "websocket_server.h"
#include "../core/logger.h"

// Simple SHA1 for WebSocket handshake
#include <openssl/sha.h>

static const char *WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static int base64_encode(const unsigned char *input, int length, char *output, int output_size) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    
    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (int k = 0; k < 4 && j < output_size - 1; k++)
                output[j++] = base64_chars[char_array_4[k]];
            i = 0;
        }
    }
    
    if (i) {
        for (int k = i; k < 3; k++)
            char_array_3[k] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int k = 0; k < i + 1 && j < output_size - 1; k++)
            output[j++] = base64_chars[char_array_4[k]];
        
        while (i++ < 3 && j < output_size - 1)
            output[j++] = '=';
    }
    
    output[j] = '\0';
    return j;
}

static void compute_websocket_accept(const char *key, char *accept) {
    char concat[256];
    unsigned char hash[SHA_DIGEST_LENGTH];
    
    snprintf(concat, sizeof(concat), "%s%s", key, WS_GUID);
    SHA1((unsigned char*)concat, strlen(concat), hash);
    base64_encode(hash, SHA_DIGEST_LENGTH, accept, 256);
}

int ws_server_init(ws_server_t *server, int port, app_config_t *config, memory_store_t *memory) {
    if (!server) return -1;
    
    memset(server, 0, sizeof(ws_server_t));
    server->port = port;
    server->config = config;
    server->memory = memory;
    
    // Create socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        LOG_ERROR("Failed to create socket");
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
        LOG_ERROR("Failed to bind to port %d", port);
        close(server->server_socket);
        return -1;
    }
    
    // Listen
    if (listen(server->server_socket, 5) < 0) {
        LOG_ERROR("Failed to listen");
        close(server->server_socket);
        return -1;
    }
    
    LOG_INFO("WebSocket server initialized on port %d", port);
    return 0;
}

void ws_server_close(ws_server_t *server) {
    if (!server) return;
    
    ws_server_stop(server);
    
    if (server->server_socket >= 0) {
        close(server->server_socket);
        server->server_socket = -1;
    }
    
    LOG_INFO("WebSocket server closed");
}

static void* ws_client_handler(void *arg) {
    ws_client_t *client = (ws_client_t *)arg;
    char buffer[WS_BUFFER_SIZE];
    
    LOG_INFO("WebSocket client connected");
    
    // Send welcome message
    const char *welcome = "{\"type\": \"connected\", \"message\": \"Welcome to MimiClaw WebSocket\"}";
    // TODO: Implement proper WebSocket frame encoding
    send(client->socket, welcome, strlen(welcome), 0);
    
    while (client->connected) {
        int received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }
        buffer[received] = '\0';
        
        LOG_DEBUG("Received: %s", buffer);
        
        // Echo back (simplified - should parse WebSocket frames)
        // TODO: Implement proper WebSocket frame parsing
    }
    
    client->connected = false;
    close(client->socket);
    LOG_INFO("WebSocket client disconnected");
    
    return NULL;
}

static void* ws_accept_loop(void *arg) {
    ws_server_t *server = (ws_server_t *)arg;
    
    LOG_INFO("WebSocket accept loop started");
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_socket = accept(server->server_socket, 
                                   (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            if (server->running) {
                LOG_ERROR("Accept failed");
            }
            continue;
        }
        
        // Find free client slot
        int slot = -1;
        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
            if (!server->clients[i].connected) {
                slot = i;
                break;
            }
        }
        
        if (slot < 0) {
            LOG_WARN("Max clients reached, rejecting connection");
            close(client_socket);
            continue;
        }
        
        // Handle WebSocket handshake (simplified)
        char buffer[1024];
        int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            
            // Extract Sec-WebSocket-Key
            char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
            if (key_start) {
                key_start += 19;
                char *key_end = strstr(key_start, "\r\n");
                if (key_end) {
                    *key_end = '\0';
                    
                    char accept_key[256];
                    compute_websocket_accept(key_start, accept_key);
                    
                    char response[512];
                    snprintf(response, sizeof(response),
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: %s\r\n"
                        "\r\n", accept_key);
                    
                    send(client_socket, response, strlen(response), 0);
                    
                    // Initialize client
                    server->clients[slot].socket = client_socket;
                    server->clients[slot].connected = true;
                    snprintf(server->clients[slot].session_id, 
                             sizeof(server->clients[slot].session_id),
                             "ws_%d_%ld", slot, time(NULL));
                    
                    pthread_create(&server->clients[slot].thread, NULL, 
                                  ws_client_handler, &server->clients[slot]);
                    pthread_detach(server->clients[slot].thread);
                }
            }
        }
    }
    
    return NULL;
}

int ws_server_start(ws_server_t *server) {
    if (!server || server->running) return -1;
    
    server->running = true;
    
    if (pthread_create(&server->accept_thread, NULL, ws_accept_loop, server) != 0) {
        server->running = false;
        LOG_ERROR("Failed to create accept thread");
        return -1;
    }
    
    pthread_detach(server->accept_thread);
    LOG_INFO("WebSocket server started");
    return 0;
}

void ws_server_stop(ws_server_t *server) {
    if (!server) return;
    
    server->running = false;
    
    // Disconnect all clients
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            server->clients[i].connected = false;
            close(server->clients[i].socket);
        }
    }
    
    LOG_INFO("WebSocket server stopped");
}

int ws_server_broadcast(ws_server_t *server, const char *message) {
    if (!server || !message) return -1;
    
    int sent = 0;
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            // TODO: Implement proper WebSocket frame encoding
            if (send(server->clients[i].socket, message, strlen(message), 0) > 0) {
                sent++;
            }
        }
    }
    
    return sent;
}