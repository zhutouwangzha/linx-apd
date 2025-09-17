#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "linx_thread_base_addr.h"
#include "linx_hash_map.h"
#include "linx_log.h"

/* 全局线程特定数据键 */
static pthread_key_t g_base_addr_key;
static bool g_base_addr_key_created = false;

/**
 * 线程退出时的清理函数
 */
static void thread_base_addr_destructor(void *context)
{
    thread_base_addr_context_t *ctx = (thread_base_addr_context_t *)context;
    
    if (ctx) {
        thread_base_addr_entry_t *current, *tmp;
        
        /* 清理base_addr映射表 */
        HASH_ITER(hh, ctx->base_addrs, current, tmp) {
            HASH_DEL(ctx->base_addrs, current);
            free(current->table_name);
            free(current);
        }
        
        free(ctx);
    }
}

int linx_thread_base_addr_init(void)
{
    int ret;
    
    if (g_base_addr_key_created) {
        LINX_LOG_WARNING("Thread base_addr system already initialized");
        return 0;
    }
    
    ret = pthread_key_create(&g_base_addr_key, thread_base_addr_destructor);
    if (ret != 0) {
        LINX_LOG_ERROR("Failed to create thread-specific key for base_addr: %s", strerror(ret));
        return -1;
    }
    
    g_base_addr_key_created = true;
    LINX_LOG_DEBUG("Thread base_addr system initialized");
    return 0;
}

void linx_thread_base_addr_deinit(void)
{
    if (g_base_addr_key_created) {
        pthread_key_delete(g_base_addr_key);
        g_base_addr_key_created = false;
        LINX_LOG_DEBUG("Thread base_addr system deinitialized");
    }
}

int linx_thread_base_addr_create(void)
{
    thread_base_addr_context_t *ctx;
    
    if (!g_base_addr_key_created) {
        LINX_LOG_ERROR("Thread base_addr system not initialized");
        return -1;
    }
    
    /* 检查是否已经存在上下文 */
    ctx = pthread_getspecific(g_base_addr_key);
    if (ctx) {
        LINX_LOG_WARNING("Thread base_addr context already exists for current thread");
        return 0;
    }
    
    /* 创建新的上下文 */
    ctx = calloc(1, sizeof(thread_base_addr_context_t));
    if (!ctx) {
        LINX_LOG_ERROR("Failed to allocate thread base_addr context");
        return -1;
    }
    
    ctx->thread_id = pthread_self();
    ctx->base_addrs = NULL;
    ctx->initialized = true;
    
    /* 设置线程特定数据 */
    if (pthread_setspecific(g_base_addr_key, ctx) != 0) {
        LINX_LOG_ERROR("Failed to set thread-specific base_addr data");
        free(ctx);
        return -1;
    }
    
    LINX_LOG_DEBUG("Created thread base_addr context for thread %lu", 
                   (unsigned long)ctx->thread_id);
    
    return 0;
}

void linx_thread_base_addr_destroy(void)
{
    thread_base_addr_context_t *ctx;
    
    if (!g_base_addr_key_created) {
        return;
    }
    
    ctx = pthread_getspecific(g_base_addr_key);
    if (ctx) {
        pthread_setspecific(g_base_addr_key, NULL);
        thread_base_addr_destructor(ctx);
        LINX_LOG_DEBUG("Destroyed thread base_addr context for thread %lu", 
                       (unsigned long)pthread_self());
    }
}

int linx_thread_base_addr_set(const char *table_name, void *base_addr)
{
    thread_base_addr_context_t *ctx;
    thread_base_addr_entry_t *entry;
    
    if (!g_base_addr_key_created || !table_name) {
        return -1;
    }
    
    ctx = pthread_getspecific(g_base_addr_key);
    if (!ctx || !ctx->initialized) {
        LINX_LOG_ERROR("Thread base_addr context not initialized");
        return -1;
    }
    
    /* 查找是否已存在该表的映射 */
    HASH_FIND_STR(ctx->base_addrs, table_name, entry);
    if (entry) {
        /* 更新现有映射 */
        entry->base_addr = base_addr;
        return 0;
    }
    
    /* 创建新的映射条目 */
    entry = malloc(sizeof(thread_base_addr_entry_t));
    if (!entry) {
        LINX_LOG_ERROR("Failed to allocate base_addr entry");
        return -1;
    }
    
    entry->table_name = strdup(table_name);
    if (!entry->table_name) {
        free(entry);
        return -1;
    }
    
    entry->base_addr = base_addr;
    
    /* 添加到哈希表 */
    HASH_ADD_STR(ctx->base_addrs, table_name, entry);
    
    LINX_LOG_DEBUG("Set base_addr for table '%s' in thread %lu", 
                   table_name, (unsigned long)ctx->thread_id);
    
    return 0;
}

void *linx_thread_base_addr_get(const char *table_name)
{
    thread_base_addr_context_t *ctx;
    thread_base_addr_entry_t *entry;
    
    if (!g_base_addr_key_created || !table_name) {
        return NULL;
    }
    
    ctx = pthread_getspecific(g_base_addr_key);
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    
    HASH_FIND_STR(ctx->base_addrs, table_name, entry);
    return entry ? entry->base_addr : NULL;
}

int linx_thread_base_addr_set_batch(field_update_table_t *tables, size_t num_tables)
{
    if (!tables) {
        return -1;
    }
    
    for (size_t i = 0; i < num_tables; i++) {
        if (linx_thread_base_addr_set(tables[i].table_name, tables[i].base_addr) != 0) {
            LINX_LOG_WARNING("Failed to set base_addr for table '%s' at index %zu", 
                           tables[i].table_name, i);
            return (int)i;
        }
    }
    
    return 0;
}