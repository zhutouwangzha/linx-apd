#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "linx_hash_map.h"

static linx_hash_map_t *s_linx_hash_map = NULL;

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
    
    /* 销毁读写锁 */
    pthread_rwlock_destroy(&table->rwlock);
    
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
    new_table->fields = NULL;
    new_table->base_addr = base_addr;  /* 可以为NULL，表示延迟绑定 */
    
    /* 初始化读写锁 */
    if (pthread_rwlock_init(&new_table->rwlock, NULL) != 0) {
        free(new_table->table_name);
        free(new_table);
        return -1;
    }

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

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, field_type_t type)
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
    char *table_name, *field_name;
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

    return linx_hash_map_get_field(table_name, field_name);
}

/* 更新指定表的基地址 */
int linx_hash_map_update_base_addr(const char *table_name, void *new_base_addr)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;  /* 表不存在 */
    }

    table->base_addr = new_base_addr;
    return 0;
}

/* 获取指定表的基地址 */
void *linx_hash_map_get_base_addr(const char *table_name)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return NULL;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return NULL;  /* 表不存在 */
    }

    return table->base_addr;
}

/* 列出所有表名 */
int linx_hash_map_list_tables(char ***table_names, size_t *count)
{
    field_table_t *current_table, *tmp_table;
    char **names;
    size_t table_count = 0;
    size_t index = 0;

    if (!s_linx_hash_map || !table_names || !count) {
        return -1;
    }

    /* 首先计算表的数量 */
    HASH_ITER(hh, s_linx_hash_map->tables, current_table, tmp_table) {
        table_count++;
    }

    if (table_count == 0) {
        *table_names = NULL;
        *count = 0;
        return 0;
    }

    /* 分配内存存储表名指针 */
    names = (char **)malloc(table_count * sizeof(char *));
    if (!names) {
        return -1;
    }

    /* 复制表名 */
    HASH_ITER(hh, s_linx_hash_map->tables, current_table, tmp_table) {
        names[index] = strdup(current_table->table_name);
        if (!names[index]) {
            /* 清理已分配的内存 */
            for (size_t i = 0; i < index; i++) {
                free(names[i]);
            }
            free(names);
            return -1;
        }
        index++;
    }

    *table_names = names;
    *count = table_count;
    return 0;
}

/* 释放表名列表 */
void linx_hash_map_free_table_list(char **table_names, size_t count)
{
    if (!table_names) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(table_names[i]);
    }
    free(table_names);
}

/* ========== 线程安全API实现 ========== */

/* 获取线程安全的数据访问上下文（读锁） */
access_context_t linx_hash_map_lock_table_read(const char *table_name)
{
    access_context_t context = {0};
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return context;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return context;
    }

    /* 获取读锁 */
    if (pthread_rwlock_rdlock(&table->rwlock) != 0) {
        return context;
    }

    /* 在锁保护下获取基地址快照 */
    context.base_addr = table->base_addr;
    context.table = table;
    context.locked = true;

    return context;
}

/* 释放数据访问上下文（解锁） */
void linx_hash_map_unlock_context(access_context_t *context)
{
    if (!context || !context->locked || !context->table) {
        return;
    }

    pthread_rwlock_unlock(&context->table->rwlock);
    
    /* 清理上下文 */
    context->base_addr = NULL;
    context->table = NULL;
    context->locked = false;
}

/* 原子更新基地址（需要写锁） */
int linx_hash_map_update_base_addr_safe(const char *table_name, void *new_base_addr)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;  /* 表不存在 */
    }

    /* 获取写锁 */
    if (pthread_rwlock_wrlock(&table->rwlock) != 0) {
        return -1;
    }

    /* 在锁保护下更新基地址 */
    table->base_addr = new_base_addr;

    /* 释放写锁 */
    pthread_rwlock_unlock(&table->rwlock);

    return 0;
}