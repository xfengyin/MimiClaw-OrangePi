/**
 * @file async_io.c
 * @brief MimiClaw Async I/O Implementation
 * @version 2.0.0
 * 
 * @brief 基于 epoll 的异步 I/O 实现 (Linux 原生)
 */

#define _GNU_SOURCE

#include "async_io.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <stdint.h>

/* ============================================================================
 * 内部结构
 * ============================================================================ */

#define MIMI_ASYNC_MAX_EVENTS 256
#define MIMI_ASYNC_MAX_FDS 1024

/**
 * @brief 任务条目
 */
typedef struct {
    int fd;
    int events;
    mimi_async_callback_t callback;
    void *user_data;
    int active;  /* 是否激活 */
} mimi_async_entry_t;

/**
 * @brief 异步上下文内部结构
 */
struct mimi_async_ctx {
    int epoll_fd;              /* epoll 文件描述符 */
    int event_fd;              /* 用于停止的事件 fd */
    mimi_async_entry_t *entries; /* 任务条目数组 */
    size_t entry_count;        /* 当前任务数 */
    size_t entry_capacity;     /* 数组容量 */
    int running;               /* 是否运行中 */
};

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

static mimi_async_entry_t* find_entry(mimi_async_ctx_t *ctx, int fd)
{
    if (ctx == NULL || ctx->entries == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < ctx->entry_count; i++) {
        if (ctx->entries[i].active && ctx->entries[i].fd == fd) {
            return &ctx->entries[i];
        }
    }
    
    return NULL;
}

static int ensure_capacity(mimi_async_ctx_t *ctx, size_t needed)
{
    if (ctx->entry_capacity >= needed) {
        return 0;
    }
    
    size_t new_capacity = ctx->entry_capacity * 2;
    if (new_capacity < needed) {
        new_capacity = needed;
    }
    if (new_capacity > MIMI_ASYNC_MAX_FDS) {
        return -1;
    }
    
    mimi_async_entry_t *new_entries = realloc(ctx->entries, 
                                               new_capacity * sizeof(mimi_async_entry_t));
    if (new_entries == NULL) {
        return -1;
    }
    
    /* 初始化新条目 */
    for (size_t i = ctx->entry_capacity; i < new_capacity; i++) {
        new_entries[i].active = 0;
        new_entries[i].fd = -1;
    }
    
    ctx->entries = new_entries;
    ctx->entry_capacity = new_capacity;
    return 0;
}

static int events_to_epoll(int events)
{
    int epoll_events = 0;
    
    if (events & MIMI_ASYNC_EVENT_READ) {
        epoll_events |= EPOLLIN;
    }
    if (events & MIMI_ASYNC_EVENT_WRITE) {
        epoll_events |= EPOLLOUT;
    }
    if (events & (MIMI_ASYNC_EVENT_ERROR | MIMI_ASYNC_EVENT_HUP)) {
        epoll_events |= (EPOLLERR | EPOLLHUP);
    }
    
    return epoll_events | EPOLLET;  /* 边缘触发 */
}

/* ============================================================================
 * 生命周期管理
 * ============================================================================ */

mimi_async_ctx_t* mimi_async_init(void)
{
    mimi_async_ctx_t *ctx = (mimi_async_ctx_t*)calloc(1, sizeof(mimi_async_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* 创建 epoll 实例 */
    ctx->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ctx->epoll_fd < 0) {
        free(ctx);
        return NULL;
    }
    
    /* 创建事件 fd 用于停止 */
    ctx->event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (ctx->event_fd < 0) {
        close(ctx->epoll_fd);
        free(ctx);
        return NULL;
    }
    
    /* 注册事件 fd */
    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = ctx->event_fd;
    
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, ctx->event_fd, &ev) < 0) {
        close(ctx->event_fd);
        close(ctx->epoll_fd);
        free(ctx);
        return NULL;
    }
    
    /* 初始化条目数组 */
    ctx->entry_capacity = 64;
    ctx->entries = (mimi_async_entry_t*)calloc(ctx->entry_capacity, sizeof(mimi_async_entry_t));
    if (ctx->entries == NULL) {
        close(ctx->event_fd);
        close(ctx->epoll_fd);
        free(ctx);
        return NULL;
    }
    
    ctx->running = 1;
    return ctx;
}

void mimi_async_destroy(mimi_async_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* 移除所有注册的 fd */
    for (size_t i = 0; i < ctx->entry_count; i++) {
        if (ctx->entries[i].active && ctx->entries[i].fd != ctx->event_fd) {
            epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, ctx->entries[i].fd, NULL);
        }
    }
    
    free(ctx->entries);
    close(ctx->event_fd);
    close(ctx->epoll_fd);
    free(ctx);
}

/* ============================================================================
 * 事件注册
 * ============================================================================ */

int mimi_async_add_fd(mimi_async_ctx_t *ctx, int fd, int events,
                      mimi_async_callback_t callback, void *user_data)
{
    if (ctx == NULL || fd < 0 || callback == NULL) {
        return -1;
    }
    
    /* 检查是否已存在 */
    if (find_entry(ctx, fd) != NULL) {
        return -1;  /* 已存在 */
    }
    
    /* 确保容量 */
    if (ensure_capacity(ctx, ctx->entry_count + 1) < 0) {
        return -1;
    }
    
    /* 添加条目 */
    mimi_async_entry_t *entry = &ctx->entries[ctx->entry_count];
    entry->fd = fd;
    entry->events = events;
    entry->callback = callback;
    entry->user_data = user_data;
    entry->active = 1;
    ctx->entry_count++;
    
    /* 注册到 epoll */
    struct epoll_event ev = {0};
    ev.events = events_to_epoll(events);
    ev.data.fd = fd;
    
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        entry->active = 0;
        ctx->entry_count--;
        return -1;
    }
    
    return 0;
}

int mimi_async_mod_fd(mimi_async_ctx_t *ctx, int fd, int events)
{
    if (ctx == NULL || fd < 0) {
        return -1;
    }
    
    mimi_async_entry_t *entry = find_entry(ctx, fd);
    if (entry == NULL) {
        return -1;  /* 不存在 */
    }
    
    entry->events = events;
    
    struct epoll_event ev = {0};
    ev.events = events_to_epoll(events);
    ev.data.fd = fd;
    
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        return -1;
    }
    
    return 0;
}

int mimi_async_remove_fd(mimi_async_ctx_t *ctx, int fd)
{
    if (ctx == NULL || fd < 0) {
        return -1;
    }
    
    mimi_async_entry_t *entry = find_entry(ctx, fd);
    if (entry == NULL) {
        return -1;  /* 不存在 */
    }
    
    /* 从 epoll 移除 */
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        return -1;
    }
    
    /* 标记为空闲 */
    entry->active = 0;
    entry->fd = -1;
    
    /* 压缩数组 (可选优化) */
    /* 这里简单实现，不压缩 */
    
    return 0;
}

/* ============================================================================
 * 事件循环
 * ============================================================================ */

int mimi_async_loop(mimi_async_ctx_t *ctx, int timeout_ms)
{
    if (ctx == NULL || !ctx->running) {
        return -1;
    }
    
    struct epoll_event events[MIMI_ASYNC_MAX_EVENTS];
    int timeout = (timeout_ms < 0) ? -1 : timeout_ms;
    
    while (ctx->running) {
        int n = epoll_wait(ctx->epoll_fd, events, MIMI_ASYNC_MAX_EVENTS, timeout);
        
        if (n < 0) {
            if (errno == EINTR) {
                continue;  /* 被信号中断，重试 */
            }
            return -1;
        }
        
        if (n == 0) {
            return 0;  /* 超时 */
        }
        
        /* 处理事件 */
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            
            /* 检查是否是停止事件 */
            if (fd == ctx->event_fd) {
                uint64_t val;
                read(fd, &val, sizeof(val));  /* 清除事件 */
                if (val > 0) {
                    ctx->running = 0;
                }
                continue;
            }
            
            /* 查找回调 */
            mimi_async_entry_t *entry = find_entry(ctx, fd);
            if (entry == NULL || !entry->active) {
                continue;
            }
            
            /* 转换事件 */
            int async_events = 0;
            if (events[i].events & (EPOLLIN | EPOLLRDHUP)) {
                async_events |= MIMI_ASYNC_EVENT_READ;
            }
            if (events[i].events & EPOLLOUT) {
                async_events |= MIMI_ASYNC_EVENT_WRITE;
            }
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                async_events |= (MIMI_ASYNC_EVENT_ERROR | MIMI_ASYNC_EVENT_HUP);
            }
            
            /* 调用回调 */
            entry->callback(fd, async_events, entry->user_data);
        }
        
        return n;  /* 返回处理的事件数 */
    }
    
    return 0;
}

int mimi_async_run_once(mimi_async_ctx_t *ctx, int max_events, int timeout_ms)
{
    if (ctx == NULL || max_events <= 0) {
        return -1;
    }
    
    if (max_events > MIMI_ASYNC_MAX_EVENTS) {
        max_events = MIMI_ASYNC_MAX_EVENTS;
    }
    
    struct epoll_event events[MIMI_ASYNC_MAX_EVENTS];
    int timeout = (timeout_ms < 0) ? -1 : timeout_ms;
    
    int n = epoll_wait(ctx->epoll_fd, events, max_events, timeout);
    
    if (n < 0) {
        if (errno == EINTR) {
            return 0;
        }
        return -1;
    }
    
    if (n == 0) {
        return 0;  /* 超时 */
    }
    
    /* 处理事件 */
    int processed = 0;
    for (int i = 0; i < n && processed < max_events; i++) {
        int fd = events[i].data.fd;
        
        /* 检查是否是停止事件 */
        if (fd == ctx->event_fd) {
            uint64_t val;
            read(fd, &val, sizeof(val));
            if (val > 0) {
                ctx->running = 0;
            }
            continue;
        }
        
        /* 查找回调 */
        mimi_async_entry_t *entry = find_entry(ctx, fd);
        if (entry == NULL || !entry->active) {
            continue;
        }
        
        /* 转换事件 */
        int async_events = 0;
        if (events[i].events & (EPOLLIN | EPOLLRDHUP)) {
            async_events |= MIMI_ASYNC_EVENT_READ;
        }
        if (events[i].events & EPOLLOUT) {
            async_events |= MIMI_ASYNC_EVENT_WRITE;
        }
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            async_events |= (MIMI_ASYNC_EVENT_ERROR | MIMI_ASYNC_EVENT_HUP);
        }
        
        /* 调用回调 */
        entry->callback(fd, async_events, entry->user_data);
        processed++;
    }
    
    return processed;
}

void mimi_async_stop(mimi_async_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    ctx->running = 0;
    
    /* 写入事件 fd 唤醒 epoll_wait */
    uint64_t val = 1;
    write(ctx->event_fd, &val, sizeof(val));
}

/* ============================================================================
 * 工具函数
 * ============================================================================ */

int mimi_async_get_epoll_fd(mimi_async_ctx_t *ctx)
{
    if (ctx == NULL) {
        return -1;
    }
    return ctx->epoll_fd;
}

size_t mimi_async_task_count(mimi_async_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    return ctx->entry_count;
}
