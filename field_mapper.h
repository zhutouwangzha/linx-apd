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
    char *field_name;           // 字段名称，如 "name", "pid"
    size_t offset;              // 在结构体中的偏移量
    field_type_t type;          // 字段类型
    size_t size;                // 字段大小
    UT_hash_handle hh;          // uthash句柄
} field_info_t;

// 映射表结构（如proc表、evt表）
typedef struct {
    char *table_id;             // 表标识符，如 "proc", "evt"
    char *description;          // 表描述
    field_info_t *fields;       // 字段哈希表
    size_t field_count;         // 字段数量
    size_t capacity;            // 当前容量
    UT_hash_handle hh;          // uthash句柄
} mapping_table_t;

// 总的hash_map管理器
typedef struct {
    mapping_table_t *tables;    // 映射表的哈希表
    size_t table_count;         // 表数量
    size_t total_fields;        // 总字段数
    bool auto_expand;           // 是否自动扩容
    size_t expand_threshold;    // 扩容阈值
} hash_map_manager_t;

// 查询结果
typedef struct {
    void *value_ptr;            // 指向值的指针
    field_type_t type;          // 值的类型
    size_t size;                // 值的大小
    bool found;                 // 是否找到
    char *table_id;             // 所属表ID
    char *field_name;           // 字段名称
} field_query_result_t;

// 表统计信息
typedef struct {
    char *table_id;
    char *description;
    size_t field_count;
    size_t capacity;
    double load_factor;
} table_stats_t;

// 管理器统计信息
typedef struct {
    size_t table_count;
    size_t total_fields;
    size_t total_capacity;
    double avg_load_factor;
    table_stats_t *table_stats;
} manager_stats_t;

// ==================== 核心API ====================

// 初始化hash_map管理器
hash_map_manager_t* hash_map_manager_init(bool auto_expand, size_t expand_threshold);

// 销毁hash_map管理器
void hash_map_manager_destroy(hash_map_manager_t *manager);

// 创建映射表（如proc表、evt表）
int hash_map_manager_create_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 const char *description,
                                 size_t initial_capacity);

// 删除映射表
int hash_map_manager_remove_table(hash_map_manager_t *manager, const char *table_id);

// 添加字段映射到指定表
int hash_map_manager_add_field(hash_map_manager_t *manager,
                              const char *table_id,
                              const char *field_name,
                              size_t offset,
                              field_type_t type,
                              size_t size);

// 批量添加字段映射
typedef struct {
    const char *field_name;
    size_t offset;
    field_type_t type;
    size_t size;
} field_mapping_t;

int hash_map_manager_add_fields_batch(hash_map_manager_t *manager,
                                     const char *table_id,
                                     const field_mapping_t *mappings,
                                     size_t count);

// 查询字段值
field_query_result_t hash_map_manager_query_field(hash_map_manager_t *manager,
                                                 const char *table_id,
                                                 const char *field_name,
                                                 const void *struct_ptr);

// 获取字段信息
field_info_t* hash_map_manager_get_field_info(hash_map_manager_t *manager,
                                             const char *table_id,
                                             const char *field_name);

// 检查表是否存在
bool hash_map_manager_table_exists(hash_map_manager_t *manager, const char *table_id);

// 检查字段是否存在
bool hash_map_manager_field_exists(hash_map_manager_t *manager,
                                  const char *table_id,
                                  const char *field_name);

// ==================== 扩容管理API ====================

// 手动扩容指定表
int hash_map_manager_expand_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 size_t new_capacity);

// 自动扩容检查和执行
int hash_map_manager_auto_expand_check(hash_map_manager_t *manager);

// 设置自动扩容参数
void hash_map_manager_set_auto_expand(hash_map_manager_t *manager, 
                                     bool enable, 
                                     size_t threshold);

// 获取表的负载因子
double hash_map_manager_get_load_factor(hash_map_manager_t *manager, const char *table_id);

// ==================== 统计和调试API ====================

// 获取管理器统计信息
manager_stats_t* hash_map_manager_get_stats(hash_map_manager_t *manager);

// 释放统计信息
void hash_map_manager_free_stats(manager_stats_t *stats);

// 打印管理器信息
void hash_map_manager_print_info(hash_map_manager_t *manager);

// 打印指定表信息
void hash_map_manager_print_table(hash_map_manager_t *manager, const char *table_id);

// 列出所有表
char** hash_map_manager_list_tables(hash_map_manager_t *manager, size_t *count);

// 列出指定表的所有字段
char** hash_map_manager_list_fields(hash_map_manager_t *manager, 
                                   const char *table_id, 
                                   size_t *count);

// ==================== 辅助宏 ====================

// 获取结构体字段偏移
#define FIELD_OFFSET(struct_type, field) offsetof(struct_type, field)

// 创建字段映射结构
#define FIELD_MAPPING(struct_type, field, field_type) \
    { .field_name = #field, \
      .offset = FIELD_OFFSET(struct_type, field), \
      .type = field_type, \
      .size = sizeof(((struct_type*)0)->field) }

// 快速添加字段映射
#define ADD_FIELD_TO_TABLE(manager, table_id, struct_type, field, field_type) \
    hash_map_manager_add_field(manager, table_id, #field, \
                              FIELD_OFFSET(struct_type, field), \
                              field_type, sizeof(((struct_type*)0)->field))

// 批量字段映射宏
#define BEGIN_FIELD_MAPPINGS(struct_type) \
    field_mapping_t mappings[] = {

#define FIELD_MAP(struct_type, field, field_type) \
    FIELD_MAPPING(struct_type, field, field_type),

#define END_FIELD_MAPPINGS() \
    }; \
    size_t mapping_count = sizeof(mappings) / sizeof(mappings[0]);

// ==================== 常量定义 ====================

// 默认配置
#define DEFAULT_TABLE_CAPACITY      64
#define DEFAULT_EXPAND_THRESHOLD    75  // 百分比
#define MAX_TABLE_NAME_LENGTH       64
#define MAX_FIELD_NAME_LENGTH       128
#define MAX_DESCRIPTION_LENGTH      256

// 错误码
#define HASH_MAP_SUCCESS            0
#define HASH_MAP_ERROR_NULL_PARAM   -1
#define HASH_MAP_ERROR_TABLE_EXISTS -2
#define HASH_MAP_ERROR_TABLE_NOT_FOUND -3
#define HASH_MAP_ERROR_FIELD_EXISTS -4
#define HASH_MAP_ERROR_FIELD_NOT_FOUND -5
#define HASH_MAP_ERROR_MEMORY       -6
#define HASH_MAP_ERROR_INVALID_PARAM -7

#endif // FIELD_MAPPER_H