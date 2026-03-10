/**
 * @file test_concurrent.c
 * @brief 并发会话测试程序
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "mimi_memory.h"

#define THREAD_COUNT 10
#define MESSAGES_PER_THREAD 100

typedef struct {
    int thread_id;
    mimi_mem_pool_t *pool;
    int success_count;
    int fail_count;
} thread_arg_t;

static void* thread_func(void *arg)
{
    thread_arg_t *targ = (thread_arg_t*)arg;
    
    char session_id[64];
    snprintf(session_id, sizeof(session_id), "concurrent_session_%d", targ->thread_id);
    
    /* 创建会话 */
    if (mimi_mem_session_create(targ->pool, session_id) != MIMI_MEMORY_OK) {
        targ->fail_count++;
        return NULL;
    }
    targ->success_count++;
    
    /* 添加消息 */
    for (int i = 0; i < MESSAGES_PER_THREAD; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Thread %d, Message %d", targ->thread_id, i);
        
        if (mimi_mem_message_append(targ->pool, session_id, "user", content, NULL) == MIMI_MEMORY_OK) {
            targ->success_count++;
        } else {
            targ->fail_count++;
        }
    }
    
    /* 查询消息 */
    mimi_mem_message_t **messages;
    int count;
    if (mimi_mem_message_query(targ->pool, session_id, MESSAGES_PER_THREAD, &messages, &count) == MIMI_MEMORY_OK) {
        targ->success_count++;
        
        for (int i = 0; i < count; i++) {
            mimi_mem_message_free(messages[i]);
        }
        free(messages);
    } else {
        targ->fail_count++;
    }
    
    return NULL;
}

int main(void)
{
    printf("=== 并发会话测试 ===\n\n");
    printf("线程数：%d\n", THREAD_COUNT);
    printf("每线程消息数：%d\n", MESSAGES_PER_THREAD);
    printf("总操作数：%d\n", THREAD_COUNT * (MESSAGES_PER_THREAD + 3));
    printf("\n");
    
    /* 初始化内存池 */
    mimi_mem_config_t config = {
        .db_path = ":memory:",
        .pool_size = THREAD_COUNT,  /* 连接池大小 = 线程数 */
        .max_idle_time = 300,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *pool = mimi_mem_pool_create(&config);
    if (!pool) {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }
    
    /* 创建线程 */
    pthread_t threads[THREAD_COUNT];
    thread_arg_t args[THREAD_COUNT];
    
    printf("启动 %d 个并发线程...\n", THREAD_COUNT);
    
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].thread_id = i;
        args[i].pool = pool;
        args[i].success_count = 0;
        args[i].fail_count = 0;
        
        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            fprintf(stderr, "线程创建失败\n");
            return 1;
        }
    }
    
    /* 等待线程完成 */
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* 统计结果 */
    int total_success = 0;
    int total_fail = 0;
    
    for (int i = 0; i < THREAD_COUNT; i++) {
        total_success += args[i].success_count;
        total_fail += args[i].fail_count;
    }
    
    printf("\n结果:\n");
    printf("  成功操作：%d\n", total_success);
    printf("  失败操作：%d\n", total_fail);
    printf("  成功率：%.2f%%\n", (double)total_success / (total_success + total_fail) * 100);
    
    /* 清理 */
    mimi_mem_pool_destroy(pool);
    
    if (total_fail == 0) {
        printf("\n✅ 并发测试通过!\n");
        return 0;
    } else {
        printf("\n❌ 并发测试失败\n");
        return 1;
    }
}
