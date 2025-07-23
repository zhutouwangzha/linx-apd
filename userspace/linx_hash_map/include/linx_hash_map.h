#ifndef __LINX_HASH_MAP_H__
#define __LINX_HASH_MAP_H__ 

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#include "field_table.h"

/**
 * 汇总
*/
typedef struct {
    field_table_t *tables;
    size_t size;
    size_t capacity;
} linx_hash_map_t;

/**
 * 查询结果
*/
typedef struct {
    size_t offset;          /* 字段偏移量 */
    field_type_t type;
    size_t size;
    bool found;
    char *table_name;
    char *field_name;
} field_result_t;

/**
 * 线程安全的数据访问上下文
 * 在访问数据期间保持基地址不变
 */
typedef struct {
    void *base_addr;        /* 锁定时的基地址 */
    field_table_t *table;   /* 对应的表 */
    bool locked;            /* 是否已锁定 */
} access_context_t;

int linx_hash_map_init(void);

void linx_hash_map_deinit(void);

int linx_hash_map_create_table(const char *table_name, void *base_addr);

int linx_hash_map_remove_table(const char *table_name);

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, field_type_t type);

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name);

field_result_t linx_hash_map_get_field_by_path(char *path);

/* 新增：更新指定表的基地址 */
int linx_hash_map_update_base_addr(const char *table_name, void *new_base_addr);

/* 新增：获取指定表的基地址 */
void *linx_hash_map_get_base_addr(const char *table_name);

/* 新增：列出所有表名 */
int linx_hash_map_list_tables(char ***table_names, size_t *count);

/* 新增：释放表名列表 */
void linx_hash_map_free_table_list(char **table_names, size_t count);

/* ========== 线程安全API ========== */

/* 获取线程安全的数据访问上下文（读锁） */
access_context_t linx_hash_map_lock_table_read(const char *table_name);

/* 释放数据访问上下文（解锁） */
void linx_hash_map_unlock_context(access_context_t *context);

/* 在锁定上下文中获取字段值指针 */
static inline void *linx_hash_map_get_value_ptr_safe(const access_context_t *context, const field_result_t *field)
{
    if (!context || !context->locked || !field->found || !context->base_addr) {
        return NULL;
    }
    return (void *)((char *)context->base_addr + field->offset);
}

/* 原子更新基地址（需要写锁） */
int linx_hash_map_update_base_addr_safe(const char *table_name, void *new_base_addr);

/* ========== 兼容性API（非线程安全） ========== */

/* 新增：根据表名和field_result获取实际值指针（自动使用表的基地址） */
static inline void *linx_hash_map_get_table_value_ptr(const char *table_name, const field_result_t *field)
{
    void *base_addr;
    
    if (!field->found || !table_name) {
        return NULL;
    }
    
    base_addr = linx_hash_map_get_base_addr(table_name);
    if (!base_addr) {
        return NULL;
    }
    
    return (void *)((char *)base_addr + field->offset);
}

/* 新增：根据基地址和field_result获取实际值指针 */
static inline void *linx_hash_map_get_value_ptr(void *base_addr, const field_result_t *field)
{
    if (!field->found || !base_addr) {
        return NULL;
    }
    return (void *)((char *)base_addr + field->offset);
}

#endif /* __LINX_HASH_MAP_H__ */
