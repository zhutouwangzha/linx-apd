#ifndef __LINX_HASH_MAP_THREAD_SAFE_H__
#define __LINX_HASH_MAP_THREAD_SAFE_H__

#include <pthread.h>
#include "linx_hash_map.h"

/**
 * 线程本地存储的字段表结构
 */
typedef struct {
    char *table_name;
    field_info_t *fields;
    void *base_addr;        /* 线程本地的基地址 */
    UT_hash_handle hh;
} thread_local_field_table_t;

/**
 * 线程本地的hash map结构
 */
typedef struct {
    thread_local_field_table_t *tables;
    size_t size;
    size_t capacity;
} thread_local_linx_hash_map_t;

/**
 * 线程上下文结构
 */
typedef struct {
    pthread_t thread_id;
    thread_local_linx_hash_map_t *local_hash_map;
    UT_hash_handle hh;
} thread_context_t;

/* 线程安全的API */
int linx_hash_map_thread_safe_init(void);
void linx_hash_map_thread_safe_deinit(void);

/* 线程本地操作 */
int linx_hash_map_thread_local_update_table_base(const char *table_name, void *base_addr);
int linx_hash_map_thread_local_update_tables_base(field_update_table_t *tables, size_t num_tables);
void *linx_hash_map_thread_local_get_table_base(const char *table_name);
field_result_t linx_hash_map_thread_local_get_field(const char *table_name, const char *field_name);
field_result_t linx_hash_map_thread_local_get_field_by_path(char *path);
void *linx_hash_map_thread_local_get_value_ptr(field_result_t *field, linx_field_type_t *type);

/* 线程管理 */
int linx_hash_map_register_thread(void);
void linx_hash_map_unregister_thread(void);

#endif /* __LINX_HASH_MAP_THREAD_SAFE_H__ */