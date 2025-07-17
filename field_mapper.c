#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>

// 初始化映射器
field_mapper_t* field_mapper_init(void) {
    field_mapper_t *mapper = (field_mapper_t*)malloc(sizeof(field_mapper_t));
    if (!mapper) {
        return NULL;
    }
    mapper->tables = NULL;
    return mapper;
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

// 销毁映射器
void field_mapper_destroy(field_mapper_t *mapper) {
    if (!mapper) return;
    
    field_table_t *current_table, *tmp_table;
    HASH_ITER(hh, mapper->tables, current_table, tmp_table) {
        HASH_DEL(mapper->tables, current_table);
        destroy_field_info(current_table->fields);
        free(current_table->table_name);
        free(current_table);
    }
    free(mapper);
}

// 创建字段表
int field_mapper_create_table(field_mapper_t *mapper, const char *table_name) {
    if (!mapper || !table_name) {
        return -1;
    }
    
    // 检查表是否已存在
    field_table_t *existing_table;
    HASH_FIND_STR(mapper->tables, table_name, existing_table);
    if (existing_table) {
        return 0; // 表已存在
    }
    
    // 创建新表
    field_table_t *new_table = (field_table_t*)malloc(sizeof(field_table_t));
    if (!new_table) {
        return -1;
    }
    
    new_table->table_name = strdup(table_name);
    if (!new_table->table_name) {
        free(new_table);
        return -1;
    }
    
    new_table->fields = NULL;
    HASH_ADD_STR(mapper->tables, table_name, new_table);
    
    return 0;
}

// 添加字段映射
int field_mapper_add_field(field_mapper_t *mapper, const char *table_name,
                          const char *field_name, size_t offset, 
                          field_type_t type, size_t size) {
    if (!mapper || !table_name || !field_name) {
        return -1;
    }
    
    // 查找表
    field_table_t *table;
    HASH_FIND_STR(mapper->tables, table_name, table);
    if (!table) {
        // 如果表不存在，自动创建
        if (field_mapper_create_table(mapper, table_name) != 0) {
            return -1;
        }
        HASH_FIND_STR(mapper->tables, table_name, table);
    }
    
    // 检查字段是否已存在
    field_info_t *existing_field;
    HASH_FIND_STR(table->fields, field_name, existing_field);
    if (existing_field) {
        // 更新现有字段
        existing_field->offset = offset;
        existing_field->type = type;
        existing_field->size = size;
        return 0;
    }
    
    // 创建新字段
    field_info_t *new_field = (field_info_t*)malloc(sizeof(field_info_t));
    if (!new_field) {
        return -1;
    }
    
    new_field->field_name = strdup(field_name);
    if (!new_field->field_name) {
        free(new_field);
        return -1;
    }
    
    new_field->offset = offset;
    new_field->type = type;
    new_field->size = size;
    
    HASH_ADD_STR(table->fields, field_name, new_field);
    
    return 0;
}

// 查询字段信息
field_info_t* field_mapper_get_field_info(field_mapper_t *mapper, 
                                         const char *table_name,
                                         const char *field_name) {
    if (!mapper || !table_name || !field_name) {
        return NULL;
    }
    
    // 查找表
    field_table_t *table;
    HASH_FIND_STR(mapper->tables, table_name, table);
    if (!table) {
        return NULL;
    }
    
    // 查找字段
    field_info_t *field;
    HASH_FIND_STR(table->fields, field_name, field);
    return field;
}

// 查询字段值
field_query_result_t field_mapper_query_value(field_mapper_t *mapper,
                                             const char *table_name,
                                             const char *field_name,
                                             const void *struct_ptr) {
    field_query_result_t result = {0};
    result.found = false;
    
    if (!mapper || !table_name || !field_name || !struct_ptr) {
        return result;
    }
    
    field_info_t *field_info = field_mapper_get_field_info(mapper, table_name, field_name);
    if (!field_info) {
        return result;
    }
    
    // 计算字段地址
    result.value_ptr = (void*)((char*)struct_ptr + field_info->offset);
    result.type = field_info->type;
    result.size = field_info->size;
    result.found = true;
    
    return result;
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

// 打印表信息（调试用）
void field_mapper_print_table(field_mapper_t *mapper, const char *table_name) {
    if (!mapper || !table_name) return;
    
    field_table_t *table;
    HASH_FIND_STR(mapper->tables, table_name, table);
    if (!table) {
        printf("Table '%s' not found\n", table_name);
        return;
    }
    
    printf("Table: %s\n", table_name);
    printf("Fields:\n");
    
    field_info_t *field;
    for (field = table->fields; field != NULL; field = field->hh.next) {
        printf("  %s: offset=%zu, type=%s, size=%zu\n",
               field->field_name, field->offset, 
               get_type_name(field->type), field->size);
    }
}

// 打印所有表信息（调试用）
void field_mapper_print_all_tables(field_mapper_t *mapper) {
    if (!mapper) return;
    
    printf("All tables:\n");
    field_table_t *table;
    for (table = mapper->tables; table != NULL; table = table->hh.next) {
        field_mapper_print_table(mapper, table->table_name);
        printf("\n");
    }
}