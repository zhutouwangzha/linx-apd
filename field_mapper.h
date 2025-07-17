#ifndef FIELD_MAPPER_H
#define FIELD_MAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "uthash.h"

// 支持的数据类型
typedef enum {
    FIELD_TYPE_INT32,
    FIELD_TYPE_INT64,
    FIELD_TYPE_UINT32,
    FIELD_TYPE_UINT64,
    FIELD_TYPE_STRING,
    FIELD_TYPE_BOOL,
    FIELD_TYPE_DOUBLE,
    FIELD_TYPE_FLOAT
} field_type_t;

// 字段信息结构
typedef struct {
    char *field_name;           // 字段名称，如 "proc.name"
    size_t offset;              // 在结构体中的偏移量
    field_type_t type;          // 字段类型
    size_t size;                // 字段大小
    UT_hash_handle hh;          // uthash句柄
} field_info_t;

// 字段映射表
typedef struct {
    char *table_name;           // 表名称
    field_info_t *fields;       // 字段哈希表
    UT_hash_handle hh;          // uthash句柄
} field_table_t;

// 映射器管理器
typedef struct {
    field_table_t *tables;      // 表的哈希表
} field_mapper_t;

// 查询结果
typedef struct {
    void *value_ptr;            // 指向值的指针
    field_type_t type;          // 值的类型
    size_t size;                // 值的大小
    bool found;                 // 是否找到
} field_query_result_t;

// 初始化映射器
field_mapper_t* field_mapper_init(void);

// 销毁映射器
void field_mapper_destroy(field_mapper_t *mapper);

// 创建字段表
int field_mapper_create_table(field_mapper_t *mapper, const char *table_name);

// 添加字段映射
int field_mapper_add_field(field_mapper_t *mapper, const char *table_name,
                          const char *field_name, size_t offset, 
                          field_type_t type, size_t size);

// 查询字段信息
field_info_t* field_mapper_get_field_info(field_mapper_t *mapper, 
                                         const char *table_name,
                                         const char *field_name);

// 查询字段值
field_query_result_t field_mapper_query_value(field_mapper_t *mapper,
                                             const char *table_name,
                                             const char *field_name,
                                             const void *struct_ptr);

// 辅助宏：获取结构体字段偏移
#define FIELD_OFFSET(struct_type, field) offsetof(struct_type, field)

// 辅助宏：添加字段映射
#define ADD_FIELD_MAPPING(mapper, table, struct_type, field, type) \
    field_mapper_add_field(mapper, table, #field, \
                          FIELD_OFFSET(struct_type, field), \
                          type, sizeof(((struct_type*)0)->field))

// 打印表信息（调试用）
void field_mapper_print_table(field_mapper_t *mapper, const char *table_name);

// 打印所有表信息（调试用）
void field_mapper_print_all_tables(field_mapper_t *mapper);

#endif // FIELD_MAPPER_H