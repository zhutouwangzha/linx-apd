#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "linx_thread_context.h"
#include "linx_log.h"
#include "field_table.h"
#include "field_info.h"
#include "uthash_ext.h"

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
        if (ctx->hash_map) {
            /* 清理哈希表 */
            field_table_t *current_table, *tmp_table;
            
            HASH_ITER(hh, ctx->hash_map->tables, current_table, tmp_table) {
                HASH_DEL(ctx->hash_map->tables, current_table);
                
                /* 清理字段信息 */
                field_info_t *current_field, *tmp_field;
                HASH_ITER(hh, current_table->fields, current_field, tmp_field) {
                    HASH_DEL(current_table->fields, current_field);
                    free(current_field);
                }
                
                free(current_table->table_name);
                free(current_table);
            }
            
            free(ctx->hash_map);
        }
        
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
    
    /* 创建独立的哈希表实例 */
    ctx->hash_map = malloc(sizeof(linx_hash_map_t));
    if (!ctx->hash_map) {
        LINX_LOG_ERROR("Failed to allocate hash map for thread context");
        free(ctx);
        return NULL;
    }
    
    ctx->hash_map->tables = NULL;
    ctx->hash_map->size = 0;
    ctx->hash_map->capacity = 0;
    
    /* 设置线程特定数据 */
    if (pthread_setspecific(g_context_key, ctx) != 0) {
        LINX_LOG_ERROR("Failed to set thread-specific data");
        free(ctx->hash_map);
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

linx_hash_map_t *linx_thread_context_get_hashmap(void)
{
    linx_thread_context_t *ctx = linx_thread_context_get();
    
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    
    return ctx->hash_map;
}