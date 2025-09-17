#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "linx_event_rich_mt.h"
#include "linx_event_rich.h"
#include "linx_thread_context.h"
#include "linx_log.h"

/* 全局初始化标志 */
static bool g_mt_initialized = false;
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;

int linx_event_rich_mt_init(void)
{
    int ret;
    
    pthread_mutex_lock(&g_init_mutex);
    
    if (g_mt_initialized) {
        LINX_LOG_WARNING("Multi-thread event rich system already initialized");
        pthread_mutex_unlock(&g_init_mutex);
        return 0;
    }
    
    /* 初始化线程上下文系统 */
    ret = linx_thread_context_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize thread context system");
        pthread_mutex_unlock(&g_init_mutex);
        return -1;
    }
    
    g_mt_initialized = true;
    pthread_mutex_unlock(&g_init_mutex);
    
    LINX_LOG_INFO("Multi-thread event rich system initialized successfully");
    return 0;
}

void linx_event_rich_mt_deinit(void)
{
    pthread_mutex_lock(&g_init_mutex);
    
    if (!g_mt_initialized) {
        pthread_mutex_unlock(&g_init_mutex);
        return;
    }
    
    /* 清理线程上下文系统 */
    linx_thread_context_deinit();
    
    g_mt_initialized = false;
    pthread_mutex_unlock(&g_init_mutex);
    
    LINX_LOG_INFO("Multi-thread event rich system deinitialized");
}

int linx_event_rich_thread_init(void)
{
    linx_thread_context_t *ctx;
    int ret;
    
    if (!g_mt_initialized) {
        LINX_LOG_ERROR("Multi-thread system not initialized, call linx_event_rich_mt_init() first");
        return -1;
    }
    
    /* 为当前线程创建上下文 */
    ctx = linx_thread_context_create();
    if (!ctx) {
        LINX_LOG_ERROR("Failed to create thread context for thread %lu", 
                      (unsigned long)pthread_self());
        return -1;
    }
    
    /* 绑定字段映射（每个线程都需要） */
    ret = linx_event_rich_bind_field();
    if (ret) {
        LINX_LOG_ERROR("Failed to bind field mappings for thread %lu", 
                      (unsigned long)pthread_self());
        linx_thread_context_destroy();
        return -1;
    }
    
    LINX_LOG_DEBUG("Thread %lu initialized successfully", (unsigned long)pthread_self());
    return 0;
}

void linx_event_rich_thread_deinit(void)
{
    linx_thread_context_t *ctx = linx_thread_context_get();
    
    if (ctx) {
        event_t *evt = &ctx->evt;
        if (evt) {
            /* 清理最后的事件 */
            rich_event_clean(evt->last_event_type);
        }
        
        /* 销毁线程上下文 */
        linx_thread_context_destroy();
        
        LINX_LOG_DEBUG("Thread %lu deinitialized", (unsigned long)pthread_self());
    }
}

int linx_event_rich_mt(linx_event_t *event)
{
    if (!event) {
        return -1;
    }
    
    /* 检查线程上下文是否初始化 */
    if (!linx_thread_context_get()) {
        LINX_LOG_ERROR("Thread context not initialized for thread %lu", 
                      (unsigned long)pthread_self());
        return -1;
    }
    
    /* 使用原有的事件丰富化逻辑，但现在是线程安全的 */
    return linx_event_rich(event);
}

event_t *linx_event_rich_mt_get(void)
{
    return linx_thread_context_get_event();
}

void *linx_event_rich_worker_thread(void *arg)
{
    linx_worker_args_t *worker_args = (linx_worker_args_t *)arg;
    int ret;
    
    if (!worker_args) {
        LINX_LOG_ERROR("Invalid worker thread arguments");
        return NULL;
    }
    
    LINX_LOG_INFO("Worker thread %d started", worker_args->thread_id);
    
    /* 初始化当前线程的事件丰富化上下文 */
    ret = linx_event_rich_thread_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize thread context for worker %d", 
                      worker_args->thread_id);
        return NULL;
    }
    
    /* 主工作循环 */
    while (!*(worker_args->stop_flag)) {
        /* 
         * 这里是示例代码，实际使用时需要根据具体的事件队列实现
         * 来获取和处理事件
         */
        
        /* 示例：从队列获取事件 */
        linx_event_t *event = NULL; /* 从 worker_args->event_queue 获取事件 */
        
        if (event) {
            /* 处理事件 */
            ret = linx_event_rich_mt(event);
            if (ret) {
                LINX_LOG_WARNING("Failed to process event in worker %d", 
                               worker_args->thread_id);
            }
            
            /* 获取处理结果 */
            event_t *rich_event = linx_event_rich_mt_get();
            if (rich_event) {
                /* 处理丰富化后的事件数据 */
                /* 用户可以在这里添加自己的处理逻辑 */
            }
        } else {
            /* 没有事件时短暂休眠，避免忙等待 */
            usleep(1000); /* 1ms */
        }
    }
    
    /* 清理线程上下文 */
    linx_event_rich_thread_deinit();
    
    LINX_LOG_INFO("Worker thread %d stopped", worker_args->thread_id);
    return NULL;
}

/* 这些函数现在已经在 linx_event_rich.h 中声明 */