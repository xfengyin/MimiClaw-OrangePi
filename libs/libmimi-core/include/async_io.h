/**
 * @file async_io.h
 * @brief MimiClaw Async I/O - epoll-based Event Loop
 * @version 2.0.0
 * @date 2026-03-10
 * 
 * @copyright Copyright (c) 2026 MimiClaw Project
 * @license MIT License
 * 
 * @brief 基于 epoll 的异步 I/O，提升并发性能 (Linux 原生)
 */

#ifndef MIMI_ASYNC_IO_H
#define MIMI_ASYNC_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 符号可见性 */
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef MIMI_CORE_BUILDING_DLL
        #define MIMI_ASYNC_API __declspec(dllexport)
    #else
        #define MIMI_ASYNC_API __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define MIMI_ASYNC_API __attribute__((visibility("default")))
    #else
        #define MIMI_ASYNC_API
    #endif
#endif

/**
 * @brief 异步 I/O 上下文 opaque 结构
 */
typedef struct mimi_async_ctx mimi_async_ctx_t;

/**
 * @brief 事件类型
 */
typedef enum {
    MIMI_ASYNC_EVENT_READ = 1,      /**< 可读事件 */
    MIMI_ASYNC_EVENT_WRITE = 2,     /**< 可写事件 */
    MIMI_ASYNC_EVENT_ERROR = 4,     /**< 错误事件 */
    MIMI_ASYNC_EVENT_HUP = 8        /**< 挂起事件 */
} mimi_async_event_t;

/**
 * @brief 回调函数类型
 * 
 * @param fd 文件描述符
 * @param events 触发的事件
 * @param user_data 用户数据
 */
typedef void (*mimi_async_callback_t)(int fd, int events, void *user_data);

/**
 * @brief 异步任务结构
 */
typedef struct {
    int fd;                     /**< 文件描述符 */
    int events;                 /**< 监听事件 */
    mimi_async_callback_t cb;   /**< 回调函数 */
    void *user_data;            /**< 用户数据 */
} mimi_async_task_t;

/* ============================================================================
 * 生命周期管理
 * ============================================================================ */

/**
 * @brief 初始化异步 I/O 上下文
 * 
 * 创建 epoll 实例和事件循环
 * 
 * @return 异步上下文句柄，失败返回 NULL
 */
MIMI_ASYNC_API mimi_async_ctx_t* mimi_async_init(void);

/**
 * @brief 销毁异步 I/O 上下文
 * 
 * 关闭 epoll，释放所有资源
 * 
 * @param ctx 异步上下文 (可为 NULL)
 */
MIMI_ASYNC_API void mimi_async_destroy(mimi_async_ctx_t *ctx);

/* ============================================================================
 * 事件注册
 * ============================================================================ */

/**
 * @brief 添加文件描述符到事件循环
 * 
 * @param ctx 异步上下文
 * @param fd 文件描述符
 * @param events 监听事件 (MIMI_ASYNC_EVENT_*)
 * @param callback 回调函数
 * @param user_data 用户数据 (传递给回调)
 * @return 成功返回 0，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_add_fd(mimi_async_ctx_t *ctx, int fd,
                                     int events, mimi_async_callback_t callback,
                                     void *user_data);

/**
 * @brief 修改已注册文件描述符的事件
 * 
 * @param ctx 异步上下文
 * @param fd 文件描述符
 * @param events 新的事件
 * @return 成功返回 0，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_mod_fd(mimi_async_ctx_t *ctx, int fd, int events);

/**
 * @brief 从事件循环移除文件描述符
 * 
 * @param ctx 异步上下文
 * @param fd 文件描述符
 * @return 成功返回 0，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_remove_fd(mimi_async_ctx_t *ctx, int fd);

/* ============================================================================
 * 事件循环
 * ============================================================================ */

/**
 * @brief 运行事件循环
 * 
 * 阻塞等待事件并调用回调
 * 
 * @param ctx 异步上下文
 * @param timeout_ms 超时时间 (毫秒)，-1 表示无限等待，0 表示非阻塞
 * @return 触发的事件数，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_loop(mimi_async_ctx_t *ctx, int timeout_ms);

/**
 * @brief 运行事件循环指定次数
 * 
 * @param ctx 异步上下文
 * @param max_events 最大处理事件数
 * @param timeout_ms 每个事件的超时时间
 * @return 处理的事件数，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_run_once(mimi_async_ctx_t *ctx, int max_events, int timeout_ms);

/**
 * @brief 停止事件循环
 * 
 * @param ctx 异步上下文
 */
MIMI_ASYNC_API void mimi_async_stop(mimi_async_ctx_t *ctx);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 获取 epoll 文件描述符
 * 
 * @param ctx 异步上下文
 * @return epoll fd，失败返回 -1
 */
MIMI_ASYNC_API int mimi_async_get_epoll_fd(mimi_async_ctx_t *ctx);

/**
 * @brief 获取注册的任务数
 * 
 * @param ctx 异步上下文
 * @return 任务数量
 */
MIMI_ASYNC_API size_t mimi_async_task_count(mimi_async_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MIMI_ASYNC_IO_H */
