#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>

// ==================== 内部辅助函数 ====================

// 计算负载因子
static double calculate_load_factor(size_t used, size_t capacity) {
    if (capacity == 0) return 0.0;
    return (double)used / capacity * 100.0;
}

// 检查是否需要扩容
static bool should_expand(hash_map_manager_t *manager, mapping_table_t *table) {
    if (!manager->auto_expand) return false;
    double load_factor = calculate_load_factor(table->field_count, table->capacity);
    return load_factor >= manager->expand_threshold;
}

// 销毁字段信息
static void destroy_field_info(field_info_t *fields) {
    field_info_t *current, *tmp;
    HASH_ITER(hh, fields, current, tmp) {
        HASH_DEL(fields, current);
        free(current->field_name);
        free(current);
    }
}

// 销毁映射表
static void destroy_mapping_table(mapping_table_t *table) {
    if (!table) return;
    destroy_field_info(table->fields);
    free(table->table_id);
    free(table->description);
    free(table);
}

// ==================== 核心API实现 ====================

// 初始化hash_map管理器
hash_map_manager_t* hash_map_manager_init(bool auto_expand, size_t expand_threshold) {
    hash_map_manager_t *manager = (hash_map_manager_t*)malloc(sizeof(hash_map_manager_t));
    if (!manager) return NULL;
    
    manager->tables = NULL;
    manager->table_count = 0;
    manager->total_fields = 0;
    manager->auto_expand = auto_expand;
    manager->expand_threshold = expand_threshold > 0 ? expand_threshold : DEFAULT_EXPAND_THRESHOLD;
    
    return manager;
}

// 销毁hash_map管理器
void hash_map_manager_destroy(hash_map_manager_t *manager) {
    if (!manager) return;
    
    mapping_table_t *current_table, *tmp_table;
    HASH_ITER(hh, manager->tables, current_table, tmp_table) {
        HASH_DEL(manager->tables, current_table);
        destroy_mapping_table(current_table);
    }
    free(manager);
}

// 创建映射表
int hash_map_manager_create_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 const char *description,
                                 size_t initial_capacity) {
    if (!manager || !table_id) return HASH_MAP_ERROR_NULL_PARAM;
    
    // 检查表是否已存在
    mapping_table_t *existing_table;
    HASH_FIND_STR(manager->tables, table_id, existing_table);
    if (existing_table) return HASH_MAP_ERROR_TABLE_EXISTS;
    
    // 创建新表
    mapping_table_t *new_table = (mapping_table_t*)malloc(sizeof(mapping_table_t));
    if (!new_table) return HASH_MAP_ERROR_MEMORY;
    
    new_table->table_id = strdup(table_id);
    new_table->description = description ? strdup(description) : strdup("");
    if (!new_table->table_id || !new_table->description) {
        free(new_table->table_id);
        free(new_table->description);
        free(new_table);
        return HASH_MAP_ERROR_MEMORY;
    }
    
    new_table->fields = NULL;
    new_table->field_count = 0;
    new_table->capacity = initial_capacity > 0 ? initial_capacity : DEFAULT_TABLE_CAPACITY;
    
    HASH_ADD_STR(manager->tables, table_id, new_table);
    manager->table_count++;
    
    return HASH_MAP_SUCCESS;
}

// 删除映射表
int hash_map_manager_remove_table(hash_map_manager_t *manager, const char *table_id) {
    if (!manager || !table_id) return HASH_MAP_ERROR_NULL_PARAM;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return HASH_MAP_ERROR_TABLE_NOT_FOUND;
    
    HASH_DEL(manager->tables, table);
    manager->table_count--;
    manager->total_fields -= table->field_count;
    
    destroy_mapping_table(table);
    return HASH_MAP_SUCCESS;
}

// 添加字段映射到指定表
int hash_map_manager_add_field(hash_map_manager_t *manager,
                              const char *table_id,
                              const char *field_name,
                              size_t offset,
                              field_type_t type,
                              size_t size) {
    if (!manager || !table_id || !field_name) return HASH_MAP_ERROR_NULL_PARAM;
    
    // 查找表
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return HASH_MAP_ERROR_TABLE_NOT_FOUND;
    
    // 检查字段是否已存在
    field_info_t *existing_field;
    HASH_FIND_STR(table->fields, field_name, existing_field);
    if (existing_field) return HASH_MAP_ERROR_FIELD_EXISTS;
    
    // 检查是否需要扩容
    if (should_expand(manager, table)) {
        hash_map_manager_expand_table(manager, table_id, table->capacity * 2);
    }
    
    // 创建新字段
    field_info_t *new_field = (field_info_t*)malloc(sizeof(field_info_t));
    if (!new_field) return HASH_MAP_ERROR_MEMORY;
    
    new_field->field_name = strdup(field_name);
    if (!new_field->field_name) {
        free(new_field);
        return HASH_MAP_ERROR_MEMORY;
    }
    
    new_field->offset = offset;
    new_field->type = type;
    new_field->size = size;
    
    HASH_ADD_STR(table->fields, field_name, new_field);
    table->field_count++;
    manager->total_fields++;
    
    return HASH_MAP_SUCCESS;
}

// 批量添加字段映射
int hash_map_manager_add_fields_batch(hash_map_manager_t *manager,
                                     const char *table_id,
                                     const field_mapping_t *mappings,
                                     size_t count) {
    if (!manager || !table_id || !mappings || count == 0) {
        return HASH_MAP_ERROR_NULL_PARAM;
    }
    
    for (size_t i = 0; i < count; i++) {
        int result = hash_map_manager_add_field(manager, table_id,
                                              mappings[i].field_name,
                                              mappings[i].offset,
                                              mappings[i].type,
                                              mappings[i].size);
        if (result != HASH_MAP_SUCCESS && result != HASH_MAP_ERROR_FIELD_EXISTS) {
            return result; // 遇到严重错误时停止
        }
    }
    
    return HASH_MAP_SUCCESS;
}

// 查询字段值
field_query_result_t hash_map_manager_query_field(hash_map_manager_t *manager,
                                                 const char *table_id,
                                                 const char *field_name,
                                                 const void *struct_ptr) {
    field_query_result_t result = {0};
    result.found = false;
    
    if (!manager || !table_id || !field_name || !struct_ptr) {
        return result;
    }
    
    // 查找表
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return result;
    
    // 查找字段
    field_info_t *field;
    HASH_FIND_STR(table->fields, field_name, field);
    if (!field) return result;
    
    // 计算字段地址
    result.value_ptr = (void*)((char*)struct_ptr + field->offset);
    result.type = field->type;
    result.size = field->size;
    result.found = true;
    result.table_id = table->table_id;
    result.field_name = field->field_name;
    
    return result;
}

// 获取字段信息
field_info_t* hash_map_manager_get_field_info(hash_map_manager_t *manager,
                                             const char *table_id,
                                             const char *field_name) {
    if (!manager || !table_id || !field_name) return NULL;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return NULL;
    
    field_info_t *field;
    HASH_FIND_STR(table->fields, field_name, field);
    return field;
}

// 检查表是否存在
bool hash_map_manager_table_exists(hash_map_manager_t *manager, const char *table_id) {
    if (!manager || !table_id) return false;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    return table != NULL;
}

// 检查字段是否存在
bool hash_map_manager_field_exists(hash_map_manager_t *manager,
                                  const char *table_id,
                                  const char *field_name) {
    return hash_map_manager_get_field_info(manager, table_id, field_name) != NULL;
}

// ==================== 扩容管理API实现 ====================

// 手动扩容指定表
int hash_map_manager_expand_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 size_t new_capacity) {
    if (!manager || !table_id) return HASH_MAP_ERROR_NULL_PARAM;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return HASH_MAP_ERROR_TABLE_NOT_FOUND;
    
    if (new_capacity <= table->capacity) return HASH_MAP_ERROR_INVALID_PARAM;
    
    table->capacity = new_capacity;
    return HASH_MAP_SUCCESS;
}

// 自动扩容检查和执行
int hash_map_manager_auto_expand_check(hash_map_manager_t *manager) {
    if (!manager || !manager->auto_expand) return HASH_MAP_SUCCESS;
    
    mapping_table_t *table;
    for (table = manager->tables; table != NULL; table = table->hh.next) {
        if (should_expand(manager, table)) {
            hash_map_manager_expand_table(manager, table->table_id, table->capacity * 2);
        }
    }
    
    return HASH_MAP_SUCCESS;
}

// 设置自动扩容参数
void hash_map_manager_set_auto_expand(hash_map_manager_t *manager, 
                                     bool enable, 
                                     size_t threshold) {
    if (!manager) return;
    manager->auto_expand = enable;
    if (threshold > 0 && threshold <= 100) {
        manager->expand_threshold = threshold;
    }
}

// 获取表的负载因子
double hash_map_manager_get_load_factor(hash_map_manager_t *manager, const char *table_id) {
    if (!manager || !table_id) return -1.0;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return -1.0;
    
    return calculate_load_factor(table->field_count, table->capacity);
}

// ==================== 统计和调试API实现 ====================

// 获取管理器统计信息
manager_stats_t* hash_map_manager_get_stats(hash_map_manager_t *manager) {
    if (!manager) return NULL;
    
    manager_stats_t *stats = (manager_stats_t*)malloc(sizeof(manager_stats_t));
    if (!stats) return NULL;
    
    stats->table_count = manager->table_count;
    stats->total_fields = manager->total_fields;
    stats->total_capacity = 0;
    stats->avg_load_factor = 0.0;
    
    if (manager->table_count > 0) {
        stats->table_stats = (table_stats_t*)malloc(sizeof(table_stats_t) * manager->table_count);
        if (!stats->table_stats) {
            free(stats);
            return NULL;
        }
        
        mapping_table_t *table;
        size_t index = 0;
        double total_load = 0.0;
        
        for (table = manager->tables; table != NULL; table = table->hh.next) {
            stats->table_stats[index].table_id = strdup(table->table_id);
            stats->table_stats[index].description = strdup(table->description);
            stats->table_stats[index].field_count = table->field_count;
            stats->table_stats[index].capacity = table->capacity;
            stats->table_stats[index].load_factor = calculate_load_factor(table->field_count, table->capacity);
            
            stats->total_capacity += table->capacity;
            total_load += stats->table_stats[index].load_factor;
            index++;
        }
        
        stats->avg_load_factor = total_load / manager->table_count;
    } else {
        stats->table_stats = NULL;
    }
    
    return stats;
}

// 释放统计信息
void hash_map_manager_free_stats(manager_stats_t *stats) {
    if (!stats) return;
    
    if (stats->table_stats) {
        for (size_t i = 0; i < stats->table_count; i++) {
            free(stats->table_stats[i].table_id);
            free(stats->table_stats[i].description);
        }
        free(stats->table_stats);
    }
    free(stats);
}

// 获取类型名称（用于调试）
static const char* get_type_name(field_type_t type) {
    switch (type) {
        case FIELD_TYPE_INT32: return "int32";
        case FIELD_TYPE_INT64: return "int64";
        case FIELD_TYPE_UINT32: return "uint32";
        case FIELD_TYPE_UINT64: return "uint64";
        case FIELD_TYPE_STRING: return "string";
        case FIELD_TYPE_BOOL: return "bool";
        case FIELD_TYPE_DOUBLE: return "double";
        case FIELD_TYPE_FLOAT: return "float";
        default: return "unknown";
    }
}

// 打印管理器信息
void hash_map_manager_print_info(hash_map_manager_t *manager) {
    if (!manager) return;
    
    printf("=== Hash Map Manager Info ===\n");
    printf("Tables: %zu\n", manager->table_count);
    printf("Total Fields: %zu\n", manager->total_fields);
    printf("Auto Expand: %s\n", manager->auto_expand ? "enabled" : "disabled");
    printf("Expand Threshold: %zu%%\n", manager->expand_threshold);
    printf("\n");
    
    mapping_table_t *table;
    for (table = manager->tables; table != NULL; table = table->hh.next) {
        double load_factor = calculate_load_factor(table->field_count, table->capacity);
        printf("Table '%s': %zu fields, capacity %zu, load %.1f%%\n",
               table->table_id, table->field_count, table->capacity, load_factor);
        printf("  Description: %s\n", table->description);
    }
}

// 打印指定表信息
void hash_map_manager_print_table(hash_map_manager_t *manager, const char *table_id) {
    if (!manager || !table_id) return;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) {
        printf("Table '%s' not found\n", table_id);
        return;
    }
    
    double load_factor = calculate_load_factor(table->field_count, table->capacity);
    printf("=== Table '%s' ===\n", table_id);
    printf("Description: %s\n", table->description);
    printf("Fields: %zu, Capacity: %zu, Load Factor: %.1f%%\n",
           table->field_count, table->capacity, load_factor);
    printf("Fields:\n");
    
    field_info_t *field;
    for (field = table->fields; field != NULL; field = field->hh.next) {
        printf("  %s: offset=%zu, type=%s, size=%zu\n",
               field->field_name, field->offset, 
               get_type_name(field->type), field->size);
    }
}

// 列出所有表
char** hash_map_manager_list_tables(hash_map_manager_t *manager, size_t *count) {
    if (!manager || !count) return NULL;
    
    *count = manager->table_count;
    if (*count == 0) return NULL;
    
    char **table_list = (char**)malloc(sizeof(char*) * (*count));
    if (!table_list) return NULL;
    
    mapping_table_t *table;
    size_t index = 0;
    for (table = manager->tables; table != NULL; table = table->hh.next) {
        table_list[index] = strdup(table->table_id);
        index++;
    }
    
    return table_list;
}

// 列出指定表的所有字段
char** hash_map_manager_list_fields(hash_map_manager_t *manager, 
                                   const char *table_id, 
                                   size_t *count) {
    if (!manager || !table_id || !count) return NULL;
    
    mapping_table_t *table;
    HASH_FIND_STR(manager->tables, table_id, table);
    if (!table) return NULL;
    
    *count = table->field_count;
    if (*count == 0) return NULL;
    
    char **field_list = (char**)malloc(sizeof(char*) * (*count));
    if (!field_list) return NULL;
    
    field_info_t *field;
    size_t index = 0;
    for (field = table->fields; field != NULL; field = field->hh.next) {
        field_list[index] = strdup(field->field_name);
        index++;
    }
    
    return field_list;
}