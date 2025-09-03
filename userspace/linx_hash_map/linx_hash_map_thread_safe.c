#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "linx_hash_map_thread_safe.h"
#include "linx_event_rich.h"
#include "linx_event_table.h"
#include "linx_log.h"
#include "field_struct.h"

/* 全局锁保护线程上下文管理 */
static pthread_rwlock_t g_thread_context_lock = PTHREAD_RWLOCK_INITIALIZER;
static thread_context_t *g_thread_contexts = NULL;
static bool g_initialized = false;

/* 线程本地存储key */
static pthread_key_t g_thread_local_key;

/* 外部全局hash_map的只读访问 */
extern linx_hash_map_t *s_linx_hash_map;

/**
 * 线程本地数据清理函数
 */
static void thread_local_cleanup(void *data)
{
    thread_local_linx_hash_map_t *local_map = (thread_local_linx_hash_map_t *)data;
    thread_local_field_table_t *current, *tmp;
    
    if (!local_map) {
        return;
    }
    
    HASH_ITER(hh, local_map->tables, current, tmp) {
        HASH_DEL(local_map->tables, current);
        free(current->table_name);
        free(current);
    }
    
    free(local_map);
}

/**
 * 获取或创建线程本地hash map
 */
static thread_local_linx_hash_map_t *get_thread_local_hash_map(void)
{
    thread_local_linx_hash_map_t *local_map;
    
    local_map = (thread_local_linx_hash_map_t *)pthread_getspecific(g_thread_local_key);
    if (local_map) {
        return local_map;
    }
    
    /* 创建新的线程本地hash map */
    local_map = (thread_local_linx_hash_map_t *)calloc(1, sizeof(thread_local_linx_hash_map_t));
    if (!local_map) {
        LINX_LOG_ERROR("Failed to allocate thread local hash map");
        return NULL;
    }
    
    local_map->tables = NULL;
    local_map->size = 0;
    local_map->capacity = 0;
    
    if (pthread_setspecific(g_thread_local_key, local_map) != 0) {
        LINX_LOG_ERROR("Failed to set thread specific data");
        free(local_map);
        return NULL;
    }
    
    return local_map;
}

/**
 * 从全局hash map复制表结构到线程本地
 */
static int copy_table_structure_to_local(const char *table_name, 
                                        thread_local_linx_hash_map_t *local_map)
{
    field_table_t *global_table;
    thread_local_field_table_t *local_table;
    
    if (!s_linx_hash_map) {
        return -1;
    }
    
    /* 在全局hash map中查找表 */
    HASH_FIND_STR(s_linx_hash_map->tables, table_name, global_table);
    if (!global_table) {
        return -1;
    }
    
    /* 检查本地是否已存在 */
    HASH_FIND_STR(local_map->tables, table_name, local_table);
    if (local_table) {
        return 0; /* 已存在 */
    }
    
    /* 创建线程本地表副本 */
    local_table = (thread_local_field_table_t *)malloc(sizeof(thread_local_field_table_t));
    if (!local_table) {
        return -1;
    }
    
    local_table->table_name = strdup(table_name);
    local_table->fields = global_table->fields; /* 共享字段定义（只读） */
    local_table->base_addr = NULL; /* 线程本地的base_addr */
    
    HASH_ADD_STR(local_map->tables, table_name, local_table);
    local_map->size++;
    
    return 0;
}

int linx_hash_map_thread_safe_init(void)
{
    if (g_initialized) {
        return 0;
    }
    
    if (pthread_key_create(&g_thread_local_key, thread_local_cleanup) != 0) {
        LINX_LOG_ERROR("Failed to create thread local key");
        return -1;
    }
    
    g_initialized = true;
    return 0;
}

void linx_hash_map_thread_safe_deinit(void)
{
    thread_context_t *current, *tmp;
    
    if (!g_initialized) {
        return;
    }
    
    pthread_rwlock_wrlock(&g_thread_context_lock);
    
    HASH_ITER(hh, g_thread_contexts, current, tmp) {
        HASH_DEL(g_thread_contexts, current);
        free(current);
    }
    
    pthread_rwlock_unlock(&g_thread_context_lock);
    
    pthread_key_delete(g_thread_local_key);
    g_initialized = false;
}

int linx_hash_map_register_thread(void)
{
    thread_context_t *context;
    pthread_t thread_id = pthread_self();
    
    pthread_rwlock_wrlock(&g_thread_context_lock);
    
    /* 检查是否已注册 */
    HASH_FIND(hh, g_thread_contexts, &thread_id, sizeof(pthread_t), context);
    if (context) {
        pthread_rwlock_unlock(&g_thread_context_lock);
        return 0;
    }
    
    /* 创建新的线程上下文 */
    context = (thread_context_t *)malloc(sizeof(thread_context_t));
    if (!context) {
        pthread_rwlock_unlock(&g_thread_context_lock);
        return -1;
    }
    
    context->thread_id = thread_id;
    context->local_hash_map = get_thread_local_hash_map();
    
    HASH_ADD(hh, g_thread_contexts, thread_id, sizeof(pthread_t), context);
    
    pthread_rwlock_unlock(&g_thread_context_lock);
    
    LINX_LOG_DEBUG("Thread %lu registered for hash map access", (unsigned long)thread_id);
    return 0;
}

void linx_hash_map_unregister_thread(void)
{
    thread_context_t *context;
    pthread_t thread_id = pthread_self();
    
    pthread_rwlock_wrlock(&g_thread_context_lock);
    
    HASH_FIND(hh, g_thread_contexts, &thread_id, sizeof(pthread_t), context);
    if (context) {
        HASH_DEL(g_thread_contexts, context);
        free(context);
    }
    
    pthread_rwlock_unlock(&g_thread_context_lock);
    
    /* 线程本地数据会在线程结束时自动清理 */
}

int linx_hash_map_thread_local_update_table_base(const char *table_name, void *base_addr)
{
    thread_local_linx_hash_map_t *local_map;
    thread_local_field_table_t *local_table;
    
    if (!table_name) {
        return -1;
    }
    
    local_map = get_thread_local_hash_map();
    if (!local_map) {
        return -1;
    }
    
    /* 确保表结构已复制到本地 */
    if (copy_table_structure_to_local(table_name, local_map) != 0) {
        return -1;
    }
    
    /* 查找并更新本地表的base_addr */
    HASH_FIND_STR(local_map->tables, table_name, local_table);
    if (!local_table) {
        return -1;
    }
    
    local_table->base_addr = base_addr;
    return 0;
}

int linx_hash_map_thread_local_update_tables_base(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;
    
    if (!tables) {
        return -1;
    }
    
    for (size_t i = 0; i < num_tables; i++) {
        if (linx_hash_map_thread_local_update_table_base(tables[i].table_name, tables[i].base_addr) != 0) {
            ret = i;
            LINX_LOG_WARNING("update %zu[%s] thread local hash map table failed!", i, tables[i].table_name);
        }
    }
    
    return ret;
}

void *linx_hash_map_thread_local_get_table_base(const char *table_name)
{
    thread_local_linx_hash_map_t *local_map;
    thread_local_field_table_t *local_table;
    
    if (!table_name) {
        return NULL;
    }
    
    local_map = get_thread_local_hash_map();
    if (!local_map) {
        return NULL;
    }
    
    HASH_FIND_STR(local_map->tables, table_name, local_table);
    if (!local_table) {
        return NULL;
    }
    
    return local_table->base_addr;
}

field_result_t linx_hash_map_thread_local_get_field(const char *table_name, const char *field_name)
{
    thread_local_linx_hash_map_t *local_map;
    thread_local_field_table_t *local_table;
    field_info_t *field;
    field_result_t result = {0};
    
    result.found = false;
    
    if (!table_name || !field_name) {
        return result;
    }
    
    local_map = get_thread_local_hash_map();
    if (!local_map) {
        return result;
    }
    
    /* 确保表结构已复制到本地 */
    if (copy_table_structure_to_local(table_name, local_map) != 0) {
        return result;
    }
    
    HASH_FIND_STR(local_map->tables, table_name, local_table);
    if (!local_table) {
        return result;
    }
    
    HASH_FIND_STR(local_table->fields, field_name, field);
    if (!field) {
        return result;
    }
    
    result.offset = field->offset;
    result.type = field->type;
    result.size = field->size;
    result.found = true;
    result.table_name = local_table->table_name;
    result.field_name = field->key;
    
    return result;
}

field_result_t linx_hash_map_thread_local_get_field_by_path(char *path)
{
    char *table_name, *field_name, *arg;
    field_result_t result = {0};
    result.found = false;
    
    if (path == NULL) {
        return result;
    }
    
    table_name = strtok(path, ".");
    if (table_name == NULL) {
        return result;
    }
    
    field_name = strtok(NULL, ".");
    if (field_name == NULL) {
        return result;
    }
    
    result = linx_hash_map_thread_local_get_field(table_name, field_name);
    
    arg = strtok(NULL, ".");
    if (arg == NULL) {
        result.arg = NULL;
    } else {
        result.arg = strdup(arg);
    }
    
    result.event_type = &(linx_event_rich_get()->num);
    
    return result;
}

void *linx_hash_map_thread_local_get_value_ptr(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;
    
    if (!field->found) {
        return NULL;
    }
    
    base_addr = linx_hash_map_thread_local_get_table_base(field->table_name);
    if (base_addr == NULL) {
        return NULL;
    }
    
    if (field->arg) {
        char *endptr;
        long index = strtol(field->arg, &endptr, 10);
        
        if (field->type == LINX_FIELD_TYPE_STRUCT) {
            /* 如果不是纯数字，则通过名称查找下标 */
            if (*endptr != '\0') {
                for (index = 0; index < g_linx_event_table[*field->event_type].nparams; index++) {
                    if (strcmp(g_linx_event_table[*field->event_type].params[index].name, field->arg) == 0) {
                        break;
                    }
                }
            }
            
            if (index >= g_linx_event_table[*field->event_type].nparams) {
                field->arg_index = -1;
                return NULL;
            }
            
            ptr = (void *)((char *)base_addr + field->offset + index * FIELD_STRUCT_SIZE);
            *type = g_linx_event_table[*field->event_type].params[index].type;
            field->arg_index = index;
        } else {
            if (*endptr != '\0') {
                // Handle non-numeric indices for other types if needed
            }
            
            ptr = (void *)((char *)base_addr + field->offset + index * sizeof(void *));
            *type = field->type;
            field->arg_index = index;
        }
    } else {
        ptr = (void *)((char *)base_addr + field->offset);
        *type = field->type;
    }
    
    return ptr;
}