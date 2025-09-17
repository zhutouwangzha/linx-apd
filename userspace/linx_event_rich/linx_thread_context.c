#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "linx_thread_context.h"
#include "linx_log.h"
#include "linx_thread_base_addr.h"

/* 全局线程特定数据键 */
static pthread_key_t g_context_key;
static bool g_context_key_created = false;

/**
 * 线程退出时的清理函数
 */
static void thread_context_destructor(void *context)
{
    linx_thread_context_t *ctx = (linx_thread_context_t *)context;
    
    if (ctx) {
        /* 清理事件结构中动态分配的内存 */
        if (ctx->evt.args) {
            free(ctx->evt.args);
        }
        
        /* 清理事件参数中动态分配的内存 */
        for (int i = 0; i < 32; i++) {
            if (ctx->evt.arg[i].data && ctx->evt.arg[i].data != ctx->evt.rawarg[i].data) {
                free(ctx->evt.arg[i].data);
            }
        }
        
        free(ctx);
    }
}

int linx_thread_context_init(void)
{
    int ret;
    
    if (g_context_key_created) {
        LINX_LOG_WARNING("Thread context already initialized");
        return 0;
    }
    
    ret = pthread_key_create(&g_context_key, thread_context_destructor);
    if (ret != 0) {
        LINX_LOG_ERROR("Failed to create thread-specific key: %s", strerror(ret));
        return -1;
    }
    
    g_context_key_created = true;
    return 0;
}

void linx_thread_context_deinit(void)
{
    if (g_context_key_created) {
        pthread_key_delete(g_context_key);
        g_context_key_created = false;
    }
}

linx_thread_context_t *linx_thread_context_create(void)
{
    linx_thread_context_t *ctx;
    int ret;
    
    if (!g_context_key_created) {
        LINX_LOG_ERROR("Thread context system not initialized");
        return NULL;
    }
    
    /* 检查是否已经存在上下文 */
    ctx = pthread_getspecific(g_context_key);
    if (ctx) {
        LINX_LOG_WARNING("Thread context already exists for current thread");
        return ctx;
    }
    
    /* 创建新的上下文 */
    ctx = calloc(1, sizeof(linx_thread_context_t));
    if (!ctx) {
        LINX_LOG_ERROR("Failed to allocate thread context");
        return NULL;
    }
    
    /* 初始化线程ID */
    ctx->thread_id = pthread_self();
    
    /* 初始化事件结构 */
    memset(&ctx->evt, 0, sizeof(event_t));
    ctx->evt.last_event_type = -1;
    
    /* 为当前线程创建base_addr上下文 */
    ret = linx_thread_base_addr_create();
    if (ret) {
        LINX_LOG_ERROR("Failed to create thread base_addr context");
        free(ctx);
        return NULL;
    }
    
    /* 设置线程特定数据 */
    if (pthread_setspecific(g_context_key, ctx) != 0) {
        LINX_LOG_ERROR("Failed to set thread-specific data");
        linx_thread_base_addr_destroy();
        free(ctx);
        return NULL;
    }
    
    ctx->initialized = true;
    
    LINX_LOG_DEBUG("Created thread context for thread %lu", (unsigned long)ctx->thread_id);
    
    return ctx;
}

void linx_thread_context_destroy(void)
{
    linx_thread_context_t *ctx;
    
    if (!g_context_key_created) {
        return;
    }
    
    ctx = pthread_getspecific(g_context_key);
    if (ctx) {
        /* 清理线程特定的base_addr存储 */
        linx_thread_base_addr_destroy();
        
        pthread_setspecific(g_context_key, NULL);
        thread_context_destructor(ctx);
    }
}

linx_thread_context_t *linx_thread_context_get(void)
{
    if (!g_context_key_created) {
        return NULL;
    }
    
    return (linx_thread_context_t *)pthread_getspecific(g_context_key);
}

event_t *linx_thread_context_get_event(void)
{
    linx_thread_context_t *ctx = linx_thread_context_get();
    
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    
    return &ctx->evt;
}

/* linx_thread_context_get_hashmap 函数已移除，
 * 因为哈希表现在通过全局共享机制管理 */