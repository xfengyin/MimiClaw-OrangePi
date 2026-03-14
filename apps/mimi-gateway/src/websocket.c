/**
 * @file websocket.c
 * @brief MimiClaw Gateway - WebSocket Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "gateway.h"

#define WS_MAX_CLIENTS 64
#define WS_BUFFER_SIZE 8192
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

typedef struct ws_client {
    int fd;
    char ip[64];
    char session_id[64];
    struct ws_client *next;
} ws_client_t;

typedef struct {
    int server_fd;
    int port;
    ws_client_t *clients;
    int client_count;
    pthread_mutex_t mutex;
    pthread_t accept_thread;
    void (*message_handler)(const char *session_id, const char *message, char **response);
    int running;
} websocket_server_t;

static websocket_server_t *g_ws_server = NULL;

/* Base64 encode (simplified) */
static char* base64_encode(const unsigned char *input, int len) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, len);
    BIO_flush(bio);
    
    BIO_get_mem_ptr(bio, &buffer_ptr);
    
    char *result = malloc(buffer_ptr->length + 1);
    memcpy(result, buffer_ptr->data, buffer_ptr->length);
    result[buffer_ptr->length] = '\0';
    
    BIO_free_all(bio);
    return result;
}

/* Generate WebSocket accept key */
static char* ws_generate_accept(const char *key) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_MAGIC_STRING);
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)combined, strlen(combined), hash);
    
    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

/* Parse WebSocket frame */
static int ws_parse_frame(const char *data, int len, char *out, int *out_len) {
    if (len < 2) return -1;
    
    unsigned char first_byte = data[0];
    unsigned char opcode = first_byte & 0x0F;
    
    /* Only handle text frames */
    if (opcode != 0x01) return -1;
    
    unsigned char second_byte = data[1];
    int masked = (second_byte & 0x80) != 0;
    uint64_t payload_len = second_byte & 0x7F;
    
    int offset = 2;
    
    /* Extended payload length */
    if (payload_len == 126) {
        if (len < 4) return -1;
        payload_len = (data[2] << 8) | data[3];
        offset = 4;
    } else if (payload_len == 127) {
        if (len < 10) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | (unsigned char)data[2 + i];
        }
        offset = 10;
    }
    
    /* Masking key */
    char mask_key[4];
    if (masked) {
        if (len < offset + 4) return -1;
        memcpy(mask_key, data + offset, 4);
        offset += 4;
    }
    
    /* Payload */
    if (len < offset + payload_len) return -1;
    
    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            out[i] = data[offset + i] ^ mask_key[i % 4];
        }
    } else {
        memcpy(out, data + offset, payload_len);
    }
    
    out[payload_len] = '\0';
    *out_len = payload_len;
    
    return 0;
}

/* Build WebSocket frame */
static int ws_build_frame(const char *msg, int len, char *out) {
    out[0] = 0x81; /* TEXT frame, FIN */
    
    if (len <= 125) {
        out[1] = len;
        memcpy(out + 2, msg, len);
        return 2 + len;
    } else if (len <= 65535) {
        out[1] = 126;
        out[2] = (len >> 8) & 0xFF;
        out[3] = len & 0xFF;
        memcpy(out + 4, msg, len);
        return 4 + len;
    } else {
        return -1; /* Too large */
    }
}

/* Send to client */
static int ws_send(ws_client_t *client, const char *msg) {
    char frame[WS_BUFFER_SIZE];
    int frame_len = ws_build_frame(msg, strlen(msg), frame);
    
    if (frame_len > 0) {
        return send(client->fd, frame, frame_len, 0);
    }
    return -1;
}

/* Handle client connection */
static void* ws_client_thread(void *arg) {
    ws_client_t *client = (ws_client_t*)arg;
    char buffer[WS_BUFFER_SIZE];
    
    while (g_ws_server->running) {
        int n = recv(client->fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        
        char message[WS_BUFFER_SIZE];
        int msg_len = 0;
        
        if (ws_parse_frame(buffer, n, message, &msg_len) == 0) {
            if (g_ws_server->message_handler) {
                char *response = NULL;
                g_ws_server->message_handler(client->session_id, message, &response);
                
                if (response) {
                    ws_send(client, response);
                    free(response);
                }
            }
        }
    }
    
    /* Remove from list */
    pthread_mutex_lock(&g_ws_server->mutex);
    ws_client_t **pp = &g_ws_server->clients;
    while (*pp) {
        if (*pp == client) {
            *pp = client->next;
            break;
        }
        pp = &(*pp)->next;
    }
    g_ws_server->client_count--;
    pthread_mutex_unlock(&g_ws_server->mutex);
    
    close(client->fd);
    free(client);
    return NULL;
}

/* Accept thread */
static void* ws_accept_thread(void *arg) {
    websocket_server_t *server = (websocket_server_t*)arg;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server->server_fd, 
                               (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) continue;
        
        /* Check max clients */
        pthread_mutex_lock(&server->mutex);
        if (server->client_count >= WS_MAX_CLIENTS) {
            close(client_fd);
            pthread_mutex_unlock(&server->mutex);
            continue;
        }
        pthread_mutex_unlock(&server->mutex);
        
        /* Create client */
        ws_client_t *client = calloc(1, sizeof(ws_client_t));
        client->fd = client_fd;
        strncpy(client->ip, inet_ntoa(client_addr.sin_addr), sizeof(client->ip) - 1);
        snprintf(client->session_id, sizeof(client->session_id), 
                "ws_%d_%ld", client_fd, (long)time(NULL));
        
        /* Add to list */
        pthread_mutex_lock(&server->mutex);
        client->next = server->clients;
        server->clients = client;
        server->client_count++;
        pthread_mutex_unlock(&server->mutex);
        
        /* Handle in new thread */
        pthread_t tid;
        pthread_create(&tid, NULL, ws_client_thread, client);
        pthread_detach(tid);
        
        printf("[WebSocket] Client connected: %s (total: %d)\n", 
               client->ip, server->client_count);
    }
    
    return NULL;
}

/* Handle HTTP upgrade request */
static int ws_handle_upgrade(int fd, const char *request) {
    /* Find Sec-WebSocket-Key */
    const char *key_line = strstr(request, "Sec-WebSocket-Key:");
    if (!key_line) return -1;
    
    char key[64];
    sscanf(key_line, "Sec-WebSocket-Key: %s", key);
    
    char *accept = ws_generate_accept(key);
    
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept);
    
    send(fd, response, strlen(response), 0);
    free(accept);
    
    return 0;
}

/* Initialize WebSocket server */
websocket_server_t* websocket_init(int port, 
                                    void (*handler)(const char*, const char*, char**)) {
    websocket_server_t *server = calloc(1, sizeof(websocket_server_t));
    if (!server) return NULL;
    
    server->port = port;
    server->message_handler = handler;
    server->running = 0;
    pthread_mutex_init(&server->mutex, NULL);
    
    /* Create socket */
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        free(server);
        return NULL;
    }
    
    /* Allow reuse */
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server->server_fd);
        free(server);
        return NULL;
    }
    
    listen(server->server_fd, 5);
    g_ws_server = server;
    
    return server;
}

/* Start server */
int websocket_start(websocket_server_t *server) {
    if (!server) return -1;
    server->running = 1;
    return pthread_create(&server->accept_thread, NULL, ws_accept_thread, server);
}

/* Stop server */
void websocket_stop(websocket_server_t *server) {
    if (!server) return;
    server->running = 0;
    
    /* Close all clients */
    pthread_mutex_lock(&server->mutex);
    ws_client_t *client = server->clients;
    while (client) {
        ws_client_t *next = client->next;
        close(client->fd);
        free(client);
        client = next;
    }
    server->clients = NULL;
    pthread_mutex_unlock(&server->mutex);
    
    pthread_join(server->accept_thread, NULL);
    close(server->server_fd);
}

/* Cleanup */
void websocket_cleanup(websocket_server_t *server) {
    if (server) {
        pthread_mutex_destroy(&server->mutex);
        free(server);
    }
    g_ws_server = NULL;
}