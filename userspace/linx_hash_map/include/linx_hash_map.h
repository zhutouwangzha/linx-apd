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
    size_t offset;
    field_type_t type;
    size_t size;
    bool found;
    char *table_name;
    char *field_name;
} field_result_t;

/**
 * 批量添加字段映射
*/
typedef struct {
    const char *field_name;
    size_t offset;
    field_type_t type;
    size_t size;
} field_mapping_t;

/**
 * 批量更新表地址
*/
typedef struct {
    const char *table_name;
    void *base_addr;
} field_update_table_t;

/**
 * 获取结构体成员偏移
*/
#define FIELD_OFFSET(struct_type, field) offsetof(struct_type, field)

/**
 * 初始化字段映射
*/
#define FILED_MAPPING(struct_type, field, field_type)   \
    {                                                   \
        .field_name = #field,                           \
        .offset = FIELD_OFFSET(struct_type, field),     \
        .size = sizeof(((struct_type *)0)->field),      \
        .type = field_type                              \
    }

/**
 * 添加字段映射
*/
#define ADD_FILED_TO_TABLE(table_id, struct_type, field, field_type)    \
    linx_hash_map_add_field(table_id, #field, \
                            FIELD_OFFSET(struct_type, field), \
                            sizeof(((struct_type *)0)->field), \
                            field_type);

/**
 * 批量字段映射宏
*/
#define BEGIN_FIELD_MAPPINGS(table_id) \
    static field_mapping_t table_id##_mappings[] = {

#define FIELD_MAP(struct_type, field, field_type) \
    FILED_MAPPING(struct_type, field, field_type),

#define END_FIELD_MAPPINGS(table_id) \
    };  \
    static size_t table_id##_mappings_count = sizeof(table_id##_mappings) / sizeof(field_mapping_t);

int linx_hash_map_init(void);

void linx_hash_map_deinit(void);

int linx_hash_map_create_table(const char *table_name, void *base_addr);

int linx_hash_map_remove_table(const char *table_name);

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, field_type_t type);

int linx_hash_map_add_field_batch(const char *table_name, const field_mapping_t *mappings, size_t count);

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name);

field_result_t linx_hash_map_get_field_by_path(char *path);

void *linx_hash_map_get_value_ptr(const field_result_t *field);

int linx_hash_map_update_table_base(const char *table_name, void *base_addr);

int linx_hash_map_update_tables_base(field_update_table_t *tables, size_t num_tables);

void *linx_hash_map_get_table_base(const char *table_name);

int linx_hash_map_list_tables(char ***table_names, size_t *num_tables);

void linx_hash_map_free_table_list(char **table_names, size_t num_tables);

#endif /* __LINX_HASH_MAP_H__ */
