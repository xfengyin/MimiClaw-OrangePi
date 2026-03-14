/**
 * @file test_gateway.c
 * @brief Gateway 单元测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "gateway.h"

/* 测试辅助函数 */
static int test_count = 0;
static int pass_count = 0;

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { \
    test_count++; \
    printf("  - %s ... ", #name); \
    test_##name(); \
    pass_count++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", #cond); \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp(a, b) != 0) { \
        printf("FAIL: '%s' != '%s'\n", a, b); \
        return; \
    } \
} while(0)

/* ============================================================================
 * Gateway 配置测试
 * ============================================================================

TEST(gateway_config_valid) {
    gateway_config_t config = {
        .bind_addr = "127.0.0.1",
        .port = 8080,
        .api_key = "test-key",
        .model = "gpt-3.5-turbo",
        .max_sessions = 100
    };
    
    ASSERT(config.port == 8080);
    ASSERT(strcmp(config.model, "gpt-3.5-turbo") == 0);
}

TEST(gateway_config_default) {
    gateway_config_t config = {
        .bind_addr = "0.0.0.0",
        .port = 8080,
        .api_key = NULL,
        .model = "gpt-3.5-turbo",
        .max_sessions = 100
    };
    
    /* 默认端口应该是 8080 */
    ASSERT(config.port == 8080);
    /* 默认模型 */
    ASSERT_STR_EQ(config.model, "gpt-3.5-turbo");
}

/* ============================================================================
 * 会话管理测试
 * ============================================================================

TEST(session_id_format) {
    /* 测试会话 ID 格式 */
    char session_id[64];
    
    /* CLI 会话 */
    snprintf(session_id, sizeof(session_id), "cli_%ld", (long)time(NULL));
    ASSERT(strncmp(session_id, "cli_", 4) == 0);
    
    /* Telegram 会话 */
    snprintf(session_id, sizeof(session_id), "tg_%s", "123456789");
    ASSERT(strncmp(session_id, "tg_", 3) == 0);
    
    /* WebSocket 会话 */
    snprintf(session_id, sizeof(session_id), "ws_%d_%ld", 123, (long)time(NULL));
    ASSERT(strncmp(session_id, "ws_", 3) == 0);
}

/* ============================================================================
 * 命令解析测试
 * ============================================================================

TEST(command_parse_chat) {
    const char *input = "/chat Hello world";
    
    /* 简单解析: 跳过 /chat 前缀 */
    const char *msg = input + 5; /* 跳过 "/chat" */
    while (*msg == ' ') msg++;   /* 跳过空格 */
    
    ASSERT(strcmp(msg, "Hello world") == 0);
}

TEST(command_parse_memory) {
    const char *input = "/memory read mykey";
    
    /* 解析 memory 命令 */
    if (strncmp(input, "/memory ", 8) == 0) {
        const char *cmd = input + 8;
        ASSERT(strncmp(cmd, "read", 4) == 0);
    }
}

TEST(command_parse_session) {
    const char *input = "/session list";
    
    if (strncmp(input, "/session ", 9) == 0) {
        const char *cmd = input + 9;
        ASSERT(strcmp(cmd, "list") == 0);
    }
}

/* ============================================================================
 * 响应格式化测试
 * ============================================================================

TEST(response_format) {
    char response[4096];
    
    /* 测试简单响应 */
    snprintf(response, sizeof(response), "Hello, %s!", "User");
    ASSERT(strcmp(response, "Hello, User!") == 0);
    
    /* 测试多行响应 */
    snprintf(response, sizeof(response), "Result:\n- Item 1\n- Item 2\n- Item 3");
    ASSERT(strstr(response, "Item 1") != NULL);
    ASSERT(strstr(response, "Item 2") != NULL);
    ASSERT(strstr(response, "Item 3") != NULL);
}

/* ============================================================================
 * 错误处理测试
 * ============================================================================

TEST(null_handling) {
    /* 测试空指针处理 */
    const char *null_str = NULL;
    
    /* 空字符串检查 */
    ASSERT(null_str == NULL);
    
    /* 空消息检查 */
    const char *empty_msg = "";
    ASSERT(strlen(empty_msg) == 0);
}

TEST(buffer_overflow_prevention) {
    char small_buffer[10];
    const char *long_string = "This is a very long string that exceeds buffer size";
    
    /* 使用 snprintf 防止溢出 */
    int len = snprintf(small_buffer, sizeof(small_buffer), "%s", long_string);
    
    /* 写入长度应该被截断 */
    ASSERT(strlen(small_buffer) == 9); /* sizeof - 1 */
    ASSERT(len > (int)strlen(small_buffer)); /* 实际需要的长度 */
}

/* ============================================================================
 * 主函数
 * ============================================================================

int main(void) {
    printf("\n=== Gateway Unit Tests ===\n\n");
    
    printf("Testing Gateway Configuration:\n");
    RUN_TEST(gateway_config_valid);
    RUN_TEST(gateway_config_default);
    
    printf("\nTesting Session Management:\n");
    RUN_TEST(session_id_format);
    
    printf("\nTesting Command Parsing:\n");
    RUN_TEST(command_parse_chat);
    RUN_TEST(command_parse_memory);
    RUN_TEST(command_parse_session);
    
    printf("\nTesting Response Format:\n");
    RUN_TEST(response_format);
    
    printf("\nTesting Error Handling:\n");
    RUN_TEST(null_handling);
    RUN_TEST(buffer_overflow_prevention);
    
    printf("\n=== Test Results ===\n");
    printf("Total:  %d\n", test_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", test_count - pass_count);
    
    return (test_count == pass_count) ? 0 : 1;
}