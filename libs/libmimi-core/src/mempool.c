/**
 * @file mempool.c
 * @brief MimiClaw Memory Pool Implementation
 * @version 2.0.0
 * 
 * @brief 固定大小内存块池，减少 malloc/free 调用，降低内存碎片
 */

#define _GNU_SOURCE

#include "mempool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 内部结构
 * ============================================================================ */

/**
 * @brief 内存块元数据
 * 
 * 每个块前面有一个小头部，记录是否在使用中
 */
typedef struct {
    int in_use;           /**< 是否在使用中 */
    uint32_t magic;       /**< 魔数，用于调试 */
} mempool_block_header_t;

#define MEMPOOL_BLOCK_MAGIC 0xDEADBEEF

/**
 * @brief 内存池内部结构
 */
struct mimi_mempool {
    mimi_mempool_config_t config;
    void *memory;                  /**< 连续内存区域 */
    mempool_block_header_t *headers; /**< 块头部数组 */
    size_t *free_list;             /**< 空闲块索引列表 */
    size_t free_count;             /**< 当前空闲块数量 */
    size_t total_size;             /**< 总内存占用 (包括元数据) */
};

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

static inline void* get_block_ptr(mimi_mempool_t *pool, size_t index)
{
    /* 内存布局：[header][block_data][header][block_data]... */
    size_t header_size = sizeof(mempool_block_header_t);
    size_t block_offset = index * (header_size + pool->config.block_size);
    return (char*)pool->memory + block_offset + header_size;
}

static inline size_t get_block_index(mimi_mempool_t *pool, void *ptr)
{
    size_t header_size = sizeof(mempool_block_header_t);
    size_t stride = header_size + pool->config.block_size;
    size_t offset = (char*)ptr - (char*)pool->memory;
    return (offset - header_size) / stride;
}

/* ============================================================================
 * 生命周期管理
 * ============================================================================ */

mimi_mempool_t* mempool_create(const mimi_mempool_config_t *config)
{
    if (config == NULL || config->block_size == 0 || config->block_count == 0) {
        return NULL;
    }

    mimi_mempool_t *pool = (mimi_mempool_t*)calloc(1, sizeof(mimi_mempool_t));
    if (pool == NULL) {
        return NULL;
    }

    pool->config = *config;
    
    size_t header_size = sizeof(mempool_block_header_t);
    size_t block_stride = header_size + config->block_size;
    
    /* 分配连续内存：头部数组 + 数据块 */
    pool->headers = (mempool_block_header_t*)calloc(config->block_count, sizeof(mempool_block_header_t));
    if (pool->headers == NULL) {
        free(pool);
        return NULL;
    }

    pool->memory = malloc(config->block_count * block_stride);
    if (pool->memory == NULL) {
        free(pool->headers);
        free(pool);
        return NULL;
    }

    /* 初始化空闲列表 */
    pool->free_list = (size_t*)malloc(config->block_count * sizeof(size_t));
    if (pool->free_list == NULL) {
        free(pool->memory);
        free(pool->headers);
        free(pool);
        return NULL;
    }

    /* 初始化所有块为空闲 */
    for (size_t i = 0; i < config->block_count; i++) {
        pool->headers[i].in_use = 0;
        pool->headers[i].magic = MEMPOOL_BLOCK_MAGIC;
        pool->free_list[i] = i;
        
        /* 清零数据区 (如果配置要求) */
        if (config->zero_init) {
            void *block_ptr = get_block_ptr(pool, i);
            memset(block_ptr, 0, config->block_size);
        }
    }

    pool->free_count = config->block_count;
    pool->total_size = sizeof(mimi_mempool_t) + 
                       config->block_count * sizeof(mempool_block_header_t) +
                       config->block_count * sizeof(size_t) +
                       config->block_count * block_stride;

    return pool;
}

void mempool_destroy(mimi_mempool_t *pool)
{
    if (pool == NULL) {
        return;
    }

    free(pool->free_list);
    free(pool->memory);
    free(pool->headers);
    free(pool);
}

/* ============================================================================
 * 内存分配
 * ============================================================================ */

void* mempool_alloc(mimi_mempool_t *pool)
{
    if (pool == NULL || pool->free_count == 0) {
        return NULL;
    }

    /* 从空闲列表获取一个块 */
    size_t index = pool->free_list[pool->free_count - 1];
    pool->free_count--;

    /* 标记为使用中 */
    pool->headers[index].in_use = 1;

    /* 清零 (如果配置要求) */
    if (pool->config.zero_init) {
        void *block_ptr = get_block_ptr(pool, index);
        memset(block_ptr, 0, pool->config.block_size);
    }

    return get_block_ptr(pool, index);
}

void mempool_free(mimi_mempool_t *pool, void *ptr)
{
    if (pool == NULL || ptr == NULL) {
        return;
    }

    /* 验证指针是否属于此池 */
    size_t index = get_block_index(pool, ptr);
    if (index >= pool->config.block_count) {
        return; /* 无效指针 */
    }

    /* 验证魔数 */
    if (pool->headers[index].magic != MEMPOOL_BLOCK_MAGIC) {
        return; /* 内存损坏 */
    }

    /* 如果已经释放，忽略 */
    if (!pool->headers[index].in_use) {
        return;
    }

    /* 标记为空闲 */
    pool->headers[index].in_use = 0;

    /* 添加到空闲列表 */
    pool->free_list[pool->free_count] = index;
    pool->free_count++;
}

/* ============================================================================
 * 统计信息
 * ============================================================================ */

void mempool_stats(mimi_mempool_t *pool,
                   size_t *total_blocks,
                   size_t *used_blocks,
                   size_t *free_blocks)
{
    if (pool == NULL) {
        if (total_blocks) *total_blocks = 0;
        if (used_blocks) *used_blocks = 0;
        if (free_blocks) *free_blocks = 0;
        return;
    }

    if (total_blocks) *total_blocks = pool->config.block_count;
    if (used_blocks) *used_blocks = pool->config.block_count - pool->free_count;
    if (free_blocks) *free_blocks = pool->free_count;
}

size_t mempool_memory_usage(mimi_mempool_t *pool)
{
    if (pool == NULL) {
        return 0;
    }
    return pool->total_size;
}

int mempool_reset(mimi_mempool_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    /* 重置所有块为空闲 */
    for (size_t i = 0; i < pool->config.block_count; i++) {
        pool->headers[i].in_use = 0;
        pool->free_list[i] = i;
        
        if (pool->config.zero_init) {
            void *block_ptr = get_block_ptr(pool, i);
            memset(block_ptr, 0, pool->config.block_size);
        }
    }

    pool->free_count = pool->config.block_count;
    return 0;
}
