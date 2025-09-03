#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "linx_hash_map.h"
#include "uthash.h"
#include "linx_event_rich.h"
#include "linx_event_table.h"
#include "linx_log.h"
#include "field_struct.h"

/* 线程本地基地址映射 */
typedef struct thread_table_map {
    char *table_name;
    void *base_addr;
    UT_hash_handle hh;
} thread_table_map_t;

static linx_hash_map_t *s_linx_hash_map = NULL;

/* 线程本地存储键 */
static pthread_key_t g_thread_table_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

/* 清理线程本地存储 */
static void cleanup_thread_tables(void *data)
{
    thread_table_map_t *tables = (thread_table_map_t *)data;
    thread_table_map_t *current, *tmp;
    
    HASH_ITER(hh, tables, current, tmp) {
        HASH_DEL(tables, current);
        free(current->table_name);
        free(current);
    }
}

/* 创建线程本地存储键 */
static void create_thread_key(void)
{
    pthread_key_create(&g_thread_table_key, cleanup_thread_tables);
}

/* 获取线程本地基地址 */
static void *get_thread_table_base(const char *table_name)
{
    thread_table_map_t *thread_tables, *table_entry;
    
    pthread_once(&g_key_once, create_thread_key);
    
    thread_tables = (thread_table_map_t *)pthread_getspecific(g_thread_table_key);
    if (!thread_tables) {
        return NULL;
    }
    
    HASH_FIND_STR(thread_tables, table_name, table_entry);
    if (!table_entry) {
        return NULL;
    }
    
    return table_entry->base_addr;
}

/* 设置线程本地基地址 */
static int set_thread_table_base(const char *table_name, void *base_addr)
{
    thread_table_map_t *thread_tables, *table_entry;
    
    pthread_once(&g_key_once, create_thread_key);
    
    thread_tables = (thread_table_map_t *)pthread_getspecific(g_thread_table_key);
    
    /* 查找现有条目 */
    HASH_FIND_STR(thread_tables, table_name, table_entry);
    if (table_entry) {
        /* 更新现有条目 */
        table_entry->base_addr = base_addr;
        return 0;
    }
    
    /* 创建新条目 */
    table_entry = malloc(sizeof(thread_table_map_t));
    if (!table_entry) {
        return -1;
    }
    
    table_entry->table_name = strdup(table_name);
    table_entry->base_addr = base_addr;
    
    HASH_ADD_STR(thread_tables, table_name, table_entry);
    pthread_setspecific(g_thread_table_key, thread_tables);
    
    return 0;
}

/* 线程安全的基地址更新函数 */
int linx_hash_map_update_tables_base_thread_safe(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;
    
    if (!tables) {
        return -1;
    }
    
    for (size_t i = 0; i < num_tables; i++) {
        ret = set_thread_table_base(tables[i].table_name, tables[i].base_addr);
        if (ret) {
            LINX_LOG_WARNING("update thread-local %zu[%s] hash map table failed!", i, tables[i].table_name);
            return ret;
        }
    }
    
    return ret;
}

/* 线程安全的基地址获取函数 */
void *linx_hash_map_get_table_base_thread_safe(const char *table_name)
{
    void *thread_base_addr;
    
    if (!s_linx_hash_map || !table_name) {
        return NULL;
    }
    
    /* 首先尝试获取线程本地基地址 */
    thread_base_addr = get_thread_table_base(table_name);
    if (thread_base_addr) {
        return thread_base_addr;
    }
    
    /* 回退到全局基地址 */
    return linx_hash_map_get_table_base(table_name);
}

/* 线程安全的值指针获取函数 */
void *linx_hash_map_get_value_ptr_thread_safe(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    /* 使用线程安全的基地址获取 */
    base_addr = linx_hash_map_get_table_base_thread_safe(field->table_name);
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