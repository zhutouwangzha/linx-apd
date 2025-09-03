#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "linx_hash_map.h"
#include "linx_event_rich.h"
#include "linx_event_table.h"
#include "linx_log.h"
#include "field_struct.h"
#include "uthash.h"

/* 线程本地基地址映射 */
typedef struct thread_base_addr_map {
    char *table_name;
    void *base_addr;
    UT_hash_handle hh;
} thread_base_addr_map_t;

static linx_hash_map_t *s_linx_hash_map = NULL;

/* 线程本地存储键 */
static pthread_key_t g_thread_base_addr_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

/* 清理线程本地存储 */
static void cleanup_thread_base_addrs(void *data)
{
    thread_base_addr_map_t *addrs = (thread_base_addr_map_t *)data;
    thread_base_addr_map_t *current, *tmp;
    
    HASH_ITER(hh, addrs, current, tmp) {
        HASH_DEL(addrs, current);
        free(current->table_name);
        free(current);
    }
}

/* 创建线程本地存储键 */
static void create_base_addr_key(void)
{
    pthread_key_create(&g_thread_base_addr_key, cleanup_thread_base_addrs);
}

/* 原有的销毁函数保持不变 */
static void destroy_field_info(field_info_t *fields)
{
    field_info_t *current, *tmp;

    if (!fields) {
        return;
    }

    HASH_ITER(hh, fields, current, tmp) {
        HASH_DEL(fields, current);
        free(current);
    }
}

static void destroy_field_table(field_table_t *table)
{
    if (!table) {
        return;
    }

    destroy_field_info(table->fields);
    free(table->table_name);
    free(table);
}

int linx_hash_map_init(void)
{
    if (s_linx_hash_map) {
        return -1;
    }

    s_linx_hash_map = (linx_hash_map_t *)malloc(sizeof(linx_hash_map_t));
    if (s_linx_hash_map == NULL) {
        return -1;
    }

    s_linx_hash_map->tables = NULL;

    return 0;
}

void linx_hash_map_deinit(void)
{
    field_table_t *current_table, *tmp_table;

    if (s_linx_hash_map == NULL) {
        return;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current_table, tmp_table) {
        HASH_DEL(s_linx_hash_map->tables, current_table);
        destroy_field_table(current_table);
    }

    free(s_linx_hash_map);
    s_linx_hash_map = NULL;
}

int linx_hash_map_create_table(const char *table_name, void *base_addr)
{
    field_table_t *existing_table, *new_table;

    if (s_linx_hash_map == NULL || table_name == NULL) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, existing_table);
    if (existing_table) {
        return -1;
    }

    new_table = malloc(sizeof(field_table_t));
    if (new_table == NULL) {
        return -1;
    }

    new_table->table_name = strdup(table_name);
    new_table->base_addr = base_addr;   /* 可以为NULL,表示延迟绑定 */
    new_table->fields = NULL;

    HASH_ADD_STR(s_linx_hash_map->tables, table_name, new_table);

    return 0;
}

int linx_hash_map_remove_table(const char *table_name)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    HASH_DEL(s_linx_hash_map->tables, table);
    s_linx_hash_map->size--;

    return 0;
}

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, linx_field_type_t type)
{
    field_table_t *table;
    field_info_t *existing_field, *new_field;

    if (!s_linx_hash_map || !table_name || !field_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    HASH_FIND_STR(table->fields, field_name, existing_field);
    if (existing_field != NULL) {
        return -1;
    }

    new_field = malloc(sizeof(field_info_t));
    if (new_field == NULL) {
        return -1;
    }

    new_field->key = (char *)field_name;
    new_field->offset = offset;
    new_field->type = type;
    new_field->size = size;

    HASH_ADD_STR(table->fields, key, new_field);

    return 0;
}

int linx_hash_map_add_field_batch(const char *table_name, const field_mapping_t *mappings, size_t count)
{
    int ret;

    if (!s_linx_hash_map || !table_name || !mappings) {
        return -1;
    }

    ret = linx_hash_map_create_table(table_name, NULL);
    if (ret) {
        return ret;
    }

    for (size_t i = 0; i < count; i++) {
        ret = linx_hash_map_add_field(table_name, mappings[i].field_name, mappings[i].offset, mappings[i].size, mappings[i].type);
        if (ret) {
            return ret;
        }
    }

    return ret;
}

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name)
{
    field_table_t *table;
    field_info_t *field;
    field_result_t result = {0};

    result.found = false;

    if (!s_linx_hash_map || !table_name || !field_name) {
        return result;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return result;
    }

    HASH_FIND_STR(table->fields, field_name, field);
    if (!field) {
        return result;
    }

    result.offset = field->offset;
    result.type = field->type;
    result.size = field->size;
    result.found = true;
    result.table_name = table->table_name;
    result.field_name = field->key;

    return result;
}

field_result_t linx_hash_map_get_field_by_path(char *path)
{
    char *table_name, *field_name, *arg;
    field_result_t result = {0};
    result.found = false;

    if (path == NULL) {
        return  result;
    }

    table_name = strtok(path, ".");
    if (table_name == NULL) {
        return result;
    }

    field_name = strtok(NULL, ".");
    if (field_name == NULL) {
        return result;
    }

    result = linx_hash_map_get_field(table_name, field_name);

    arg = strtok(NULL, ".");
    if (arg == NULL) {
        result.arg = NULL;
    } else {
        result.arg = strdup(arg);
    }

    result.event_type = &(linx_event_rich_get()->num);

    return result;
}

/* 设置线程本地基地址 */
static int set_thread_base_addr(const char *table_name, void *base_addr)
{
    thread_base_addr_map_t *thread_addrs, *addr_entry;
    
    pthread_once(&g_key_once, create_base_addr_key);
    
    thread_addrs = (thread_base_addr_map_t *)pthread_getspecific(g_thread_base_addr_key);
    
    /* 查找现有条目 */
    HASH_FIND_STR(thread_addrs, table_name, addr_entry);
    if (addr_entry) {
        /* 更新现有条目 */
        addr_entry->base_addr = base_addr;
        return 0;
    }
    
    /* 创建新条目 */
    addr_entry = malloc(sizeof(thread_base_addr_map_t));
    if (!addr_entry) {
        return -1;
    }
    
    addr_entry->table_name = strdup(table_name);
    if (!addr_entry->table_name) {
        free(addr_entry);
        return -1;
    }
    addr_entry->base_addr = base_addr;
    
    HASH_ADD_STR(thread_addrs, table_name, addr_entry);
    pthread_setspecific(g_thread_base_addr_key, thread_addrs);
    
    return 0;
}

/* 获取线程本地基地址 */
static void *get_thread_base_addr(const char *table_name)
{
    thread_base_addr_map_t *thread_addrs, *addr_entry;
    
    pthread_once(&g_key_once, create_base_addr_key);
    
    thread_addrs = (thread_base_addr_map_t *)pthread_getspecific(g_thread_base_addr_key);
    if (!thread_addrs) {
        return NULL;
    }
    
    HASH_FIND_STR(thread_addrs, table_name, addr_entry);
    if (!addr_entry) {
        return NULL;
    }
    
    return addr_entry->base_addr;
}

/* 线程安全的基地址更新 - 使用线程本地存储 */
int linx_hash_map_update_tables_base_safe(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;
    
    if (!tables) {
        return -1;
    }
    
    /* 将基地址存储到线程本地存储中，避免全局竞态 */
    for (size_t i = 0; i < num_tables; i++) {
        ret = set_thread_base_addr(tables[i].table_name, tables[i].base_addr);
        if (ret) {
            LINX_LOG_WARNING("set thread-local %zu[%s] base addr failed!", i, tables[i].table_name);
            return ret;
        }
    }
    
    return ret;
}

/* 线程安全的基地址获取 */
void *linx_hash_map_get_table_base_safe(const char *table_name)
{
    void *thread_base_addr;
    
    if (!table_name) {
        return NULL;
    }
    
    /* 优先使用线程本地基地址 */
    thread_base_addr = get_thread_base_addr(table_name);
    if (thread_base_addr) {
        return thread_base_addr;
    }
    
    /* 回退到全局基地址 */
    return linx_hash_map_get_table_base(table_name);
}

/* 线程安全的值指针获取 */
void *linx_hash_map_get_value_ptr_safe(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    /* 使用线程安全的基地址获取 */
    base_addr = linx_hash_map_get_table_base_safe(field->table_name);
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

/* 原有函数保持不变，用于向后兼容 */
int linx_hash_map_update_table_base(const char *table_name, void *base_addr)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    table->base_addr = base_addr;

    return 0;
}

int linx_hash_map_update_tables_base(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;

    if (!s_linx_hash_map || !tables) {
        return -1;
    }

    for (size_t i = 0; i < num_tables; i++) {
        ret = linx_hash_map_update_table_base(tables[i].table_name, tables[i].base_addr);
        if (ret) {
            ret = i;
            LINX_LOG_WARNING("update %d[%s] hash map table failed!", i, tables[i].table_name);
        }
    }

    return ret;
}

void *linx_hash_map_get_table_base(const char *table_name)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return NULL;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return NULL;
    }

    return table->base_addr;
}

void *linx_hash_map_get_value_ptr(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    base_addr = linx_hash_map_get_table_base(field->table_name);
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

/* 其他原有函数保持不变 */
int linx_hash_map_list_tables(char ***table_names, size_t *num_tables)
{
    field_table_t *current, *tmp;
    char **names;
    size_t table_count = 0;
    size_t index = 0;

    if (!s_linx_hash_map || !table_names || !num_tables) {
        return -1;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current, tmp) {
        table_count++;
    }

    if (table_count == 0) {
        *table_names = NULL;
        *num_tables = 0;
        return 0;
    }

    names = malloc(sizeof(char *) * table_count);
    if (!names) {
        return -1;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current, tmp) {
        names[index] = strdup(current->table_name);
        if (!names[index]) {
            for (size_t i = 0; i < index; i++) {
                free(names[i]);
                names[i] = NULL;
            }

            free(names);
            names = NULL;
            return -1;
        }

        index++;
    }

    *table_names = names;
    *num_tables = table_count;
    return 0;
}

void linx_hash_map_free_table_list(char **table_names, size_t num_tables)
{
    if (!table_names) {
        return;
    }

    for (size_t i = 0; i < num_tables; i++) {
        free(table_names[i]);
        table_names[i] = NULL;
    }

    free(table_names);
    table_names = NULL;
}