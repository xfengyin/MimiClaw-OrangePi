/**
 * @file test_latency.c
 * @brief 响应延迟测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mimi_memory.h"

#define TEST_COUNT 1000

static int64_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(void)
{
    printf("=== 响应延迟测试 ===\n\n");
    
    /* 初始化 */
    mimi_mem_config_t config = {
        .db_path = ":memory:",
        .pool_size = 4,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    if (!pool) {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }
    
    const char *session_id = "latency_test";
    mimi_mem_session_create(pool, session_id);
    
    /* 测试写入延迟 */
    printf("1. 测试写入延迟 (%d 次)...\n", TEST_COUNT);
    
    int64_t start = get_time_ms();
    for (int i = 0; i < TEST_COUNT; i++) {
        char content[64];
        snprintf(content, sizeof(content), "Message %d", i);
        mimi_mem_message_append(pool, session_id, "user", content, NULL);
    }
    int64_t end = get_time_ms();
    
    double avg_write = (double)(end - start) / TEST_COUNT;
    printf("   平均写入延迟: %.3f ms\n", avg_write);
    
    /* 测试读取延迟 */
    printf("\n2. 测试读取延迟 (%d 次)...\n", TEST_COUNT);
    
    start = get_time_ms();
    for (int i = 0; i < TEST_COUNT / 10; i++) {  /* 读取 1/10 以减少总量 */
        mimi_mem_message_t **messages;
        int count;
        mimi_mem_message_query(pool, session_id, 10, &messages, &count);
        
        for (int j = 0; j < count; j++) {
            mimi_mem_message_free(messages[j]);
        }
        free(messages);
    }
    end = get_time_ms();
    
    double avg_read = (double)(end - start) / (TEST_COUNT / 10);
    printf("   平均读取延迟: %.3f ms\n", avg_read);
    
    /* 测试内存操作延迟 */
    printf("\n3. 测试内存操作延迟 (%d 次)...\n", TEST_COUNT);
    
    start = get_time_ms();
    for (int i = 0; i < TEST_COUNT; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        mimi_mem_write(pool, key, value);
    }
    end = get_time_ms();
    
    double avg_write_mem = (double)(end - start) / TEST_COUNT;
    printf("   平均写入延迟: %.3f ms\n", avg_write_mem);
    
    /* 清理 */
    mimi_mem_pool_destroy(pool);
    
    printf("\n=== 结果汇总 ===\n");
    printf("写入延迟：%.3f ms (目标 <0.5ms)\n", avg_write);
    printf("读取延迟：%.3f ms (目标 <0.5ms)\n", avg_read);
    printf("内存写入：%.3f ms (目标 <0.5ms)\n", avg_write_mem);
    
    if (avg_write < 0.5 && avg_read < 0.5 && avg_write_mem < 0.5) {
        printf("\n✅ 所有指标达标!\n");
    } else {
        printf("\n❌ 部分指标未达标\n");
    }
    
    return 0;
}
