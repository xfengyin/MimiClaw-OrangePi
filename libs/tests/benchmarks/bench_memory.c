/**
 * @file test_memory.c
 * @brief 内存占用测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mempool.h"
#include "mimi_memory.h"

#define TEST_ITERATIONS 10000

int main(void)
{
    printf("=== MimiClaw 内存占用测试 ===\n\n");
    
    /* 测试内存池 */
    printf("1. 测试内存池 (4KB x 256 块)...\n");
    
    mimi_mempool_config_t pool_config = {
        .block_size = 4096,
        .block_count = 256,
        .zero_init = 1
    };
    
    mimi_mempool_t *pool = mempool_create(&pool_config);
    if (!pool) {
        fprintf(stderr, "内存池创建失败\n");
        return 1;
    }
    
    size_t usage = mempool_memory_usage(pool);
    printf("   内存池占用: %zu KB\n", usage / 1024);
    
    /* 模拟分配/释放 */
    void *blocks[100];
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        for (int j = 0; j < 100; j++) {
            blocks[j] = mempool_alloc(pool);
        }
        for (int j = 0; j < 100; j++) {
            mempool_free(pool, blocks[j]);
        }
    }
    
    printf("   完成 %d 次分配/释放循环\n", TEST_ITERATIONS);
    
    mempool_destroy(pool);
    
    /* 测试 SQLite 内存池 */
    printf("\n2. 测试 SQLite 连接池...\n");
    
    mimi_mem_config_t mem_config = {
        .db_path = ":memory:",
        .pool_size = 4,
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *mem_pool = mimi_mem_pool_create(&mem_config);
    if (!mem_pool) {
        fprintf(stderr, "内存池创建失败: %s\n", mimi_mem_last_error(NULL));
        return 1;
    }
    
    printf("   SQLite 连接池创建成功 (4 连接)\n");
    
    /* 测试会话操作 */
    for (int i = 0; i < 100; i++) {
        char session_id[64];
        snprintf(session_id, sizeof(session_id), "test_session_%d", i);
        mimi_mem_session_create(mem_pool, session_id);
        
        /* 添加消息 */
        mimi_mem_message_append(mem_pool, session_id, "user", "Hello", NULL);
        mimi_mem_message_append(mem_pool, session_id, "assistant", "Hi there", NULL);
    }
    
    printf("   创建 100 个会话，每个 2 条消息\n");
    
    mimi_mem_pool_destroy(mem_pool);
    
    printf("\n✓ 内存测试完成\n");
    return 0;
}
