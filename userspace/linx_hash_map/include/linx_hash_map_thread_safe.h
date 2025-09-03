#ifndef __LINX_HASH_MAP_THREAD_SAFE_H__
#define __LINX_HASH_MAP_THREAD_SAFE_H__

#include "linx_hash_map.h"

/**
 * 线程安全的基地址更新函数
 * 在多线程环境下，每个线程维护自己的基地址副本
 */
int linx_hash_map_update_tables_base_thread_safe(field_update_table_t *tables, size_t num_tables);

/**
 * 线程安全的基地址获取函数
 * 优先返回线程本地基地址，如果不存在则回退到全局基地址
 */
void *linx_hash_map_get_table_base_thread_safe(const char *table_name);

/**
 * 线程安全的值指针获取函数
 * 使用线程本地基地址来计算字段值指针
 */
void *linx_hash_map_get_value_ptr_thread_safe(field_result_t *field, linx_field_type_t *type);

#endif /* __LINX_HASH_MAP_THREAD_SAFE_H__ */