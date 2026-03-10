/**
 * @file mempool.h
 * @brief MimiClaw Memory Pool - Efficient Block Allocation
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 * 
 * @brief 内存池实现，用于减少 malloc/free 碎片，降低内存占用
 * 
 * 使用场景:
 * - 消息缓冲区 (4KB)
 * - 会话上下文 (2KB)
 * - HTTP 响应缓冲 (16KB)
 */

#ifndef MIMI_MEMPOOL_H
#define MIMI_MEMPOOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 符号可见性 */
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef MIMI_CORE_BUILDING_DLL
        #define MIMI_MEMPOOL_API __declspec(dllexport)
    #else
        #define MIMI_MEMPOOL_API __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define MIMI_MEMPOOL_API __attribute__((visibility("default")))
    #else
        #define MIMI_MEMPOOL_API
    #endif
#endif

/**
 * @brief 内存池 opaque 结构
 * 
 * 内部管理固定大小的内存块数组
 */
typedef struct mimi_mempool mimi_mempool_t;

/**
 * @brief 内存池配置
 */
typedef struct {
    size_t block_size;    /**< 每个块的大小 (字节) */
    size_t block_count;   /**< 块的数量 */
    int zero_init;        /**< 分配时是否清零 */
} mimi_mempool_config_t;

/* ============================================================================
 * 生命周期管理
 * ============================================================================ */

/**
 * @brief 创建内存池
 * 
 * @param config 配置结构
 * @return 内存池句柄，失败返回 NULL
 * 
 * @note 线程安全：每个池独立，可在不同线程使用
 * @note 内存：调用者必须在完成后调用 mempool_destroy()
 */
MIMI_MEMPOOL_API mimi_mempool_t* mempool_create(const mimi_mempool_config_t *config);

/**
 * @brief 销毁内存池
 * 
 * 释放所有内存块和池结构
 * 
 * @param pool 内存池 (可为 NULL)
 */
MIMI_MEMPOOL_API void mempool_destroy(mimi_mempool_t *pool);

/* ============================================================================
 * 内存分配
 * ============================================================================ */

/**
 * @brief 从内存池分配一块内存
 * 
 * @param pool 内存池
 * @return 分配的内存指针，池耗尽时返回 NULL
 * 
 * @note 时间复杂度：O(1)
 * @note 线程安全：需要外部同步
 */
MIMI_MEMPOOL_API void* mempool_alloc(mimi_mempool_t *pool);

/**
 * @brief 释放内存块回池
 * 
 * @param pool 内存池
 * @param ptr 要释放的指针 (必须来自此池)
 * 
 * @note 时间复杂度：O(1)
 * @note 不会实际释放内存，仅标记为可用
 */
MIMI_MEMPOOL_API void mempool_free(mimi_mempool_t *pool, void *ptr);

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/**
 * @brief 获取池统计信息
 * 
 * @param pool 内存池
 * @param total_blocks 输出：总块数
 * @param used_blocks 输出：已使用块数
 * @param free_blocks 输出：空闲块数
 * 
 * @note 线程安全：需要外部同步
 */
MIMI_MEMPOOL_API void mempool_stats(mimi_mempool_t *pool,
                                    size_t *total_blocks,
                                    size_t *used_blocks,
                                    size_t *free_blocks);

/**
 * @brief 获取内存池占用字节数
 * 
 * @param pool 内存池
 * @return 总占用字节数 (包括元数据)
 */
MIMI_MEMPOOL_API size_t mempool_memory_usage(mimi_mempool_t *pool);

/**
 * @brief 重置内存池
 * 
 * 标记所有块为空闲 (不清除内容)
 * 
 * @param pool 内存池
 * @return 成功返回 0，失败返回 -1
 */
MIMI_MEMPOOL_API int mempool_reset(mimi_mempool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_MEMPOOL_H */
