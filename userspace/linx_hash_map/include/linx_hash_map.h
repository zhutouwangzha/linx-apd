#ifndef __LINX_HASH_MAP_H__
#define __LINX_HASH_MAP_H__ 

#include <stddef.h>
#include <stdbool.h>

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

int linx_hash_map_init(void);

void linx_hash_map_deinit(void);

int linx_hash_map_create_table(const char *table_name, void *base_addr);

int linx_hash_map_remove_table(const char *table_name);

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, field_type_t type);

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name);

field_result_t linx_hash_map_get_field_by_path(char *path);

/* 新增：根据基地址和field_result获取实际值指针 */
static inline void *linx_hash_map_get_value_ptr(void *base_addr, const field_result_t *field)
{
    if (!field->found || !base_addr) {
        return NULL;
    }
    return (void *)((char *)base_addr + field->offset);
}

#endif /* __LINX_HASH_MAP_H__ */
