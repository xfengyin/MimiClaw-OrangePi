/**
 * @file test_startup.c
 * @brief 启动时间测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mimi_memory.h"

int main(void)
{
    /* 初始化内存池 */
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
    
    /* 执行基本操作 */
    mimi_mem_session_create(pool, "startup_test");
    mimi_mem_message_append(pool, "startup_test", "user", "Test", NULL);
    
    /* 清理 */
    mimi_mem_pool_destroy(pool);
    
    return 0;
}
