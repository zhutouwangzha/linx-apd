#include <stddef.h>

#include "linx_hash_map.h"
#include "linx_event_rich.h"
#include "linx_event_table.h"
#include "linx_log.h"
#include "field_struct.h"
#include "linx_thread_base_addr.h"

/* 全局共享的哈希表实例 - 只存储字段映射信息 */
static linx_hash_map_t *g_shared_hash_map = NULL;
static pthread_mutex_t g_hash_map_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    free(table);
}

int linx_hash_map_init(void)
{
    int ret;
    
    pthread_mutex_lock(&g_hash_map_mutex);
    
    if (g_shared_hash_map) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return 0;
    }
    
    /* 初始化线程特定base_addr系统 */
    ret = linx_thread_base_addr_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize thread base_addr system");
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }
    
    /* 创建全局共享的哈希表实例 */
    g_shared_hash_map = (linx_hash_map_t *)malloc(sizeof(linx_hash_map_t));
    if (g_shared_hash_map == NULL) {
        linx_thread_base_addr_deinit();
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }
    
    g_shared_hash_map->tables = NULL;
    g_shared_hash_map->size = 0;
    g_shared_hash_map->capacity = 0;
    
    pthread_mutex_unlock(&g_hash_map_mutex);
    
    LINX_LOG_INFO("Hash map system initialized with shared field mappings");
    return 0;
}

void linx_hash_map_deinit(void)
{
    field_table_t *current_table, *tmp_table;
    
    pthread_mutex_lock(&g_hash_map_mutex);
    
    if (g_shared_hash_map == NULL) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return;
    }
    
    /* 清理共享的字段映射表 */
    HASH_ITER(hh, g_shared_hash_map->tables, current_table, tmp_table) {
        HASH_DEL(g_shared_hash_map->tables, current_table);
        destroy_field_table(current_table);
    }
    
    free(g_shared_hash_map);
    g_shared_hash_map = NULL;
    
    pthread_mutex_unlock(&g_hash_map_mutex);
    
    /* 清理线程特定base_addr系统 */
    linx_thread_base_addr_deinit();
    
    LINX_LOG_INFO("Hash map system deinitialized");
}

int linx_hash_map_create_table(const char *table_name, void *base_addr)
{
    field_table_t *existing_table, *new_table;

    if (g_shared_hash_map == NULL || table_name == NULL) {
        return -1;
    }

    pthread_mutex_lock(&g_hash_map_mutex);
    
    /* 检查共享映射表中是否已存在 */
    HASH_FIND_STR(g_shared_hash_map->tables, table_name, existing_table);
    if (existing_table) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        /* 表已存在，只需设置当前线程的base_addr */
        if (base_addr) {
            return linx_thread_base_addr_set(table_name, base_addr);
        }
        return 0;
    }

    /* 创建新的字段映射表 */
    new_table = malloc(sizeof(field_table_t));
    if (new_table == NULL) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }

    new_table->table_name = strdup(table_name);
    new_table->base_addr = NULL;  /* 共享表中不存储base_addr */
    new_table->fields = NULL;

    HASH_ADD_STR(g_shared_hash_map->tables, table_name, new_table);
    
    pthread_mutex_unlock(&g_hash_map_mutex);
    
    /* 设置当前线程的base_addr */
    if (base_addr) {
        return linx_thread_base_addr_set(table_name, base_addr);
    }

    return 0;
}

int linx_hash_map_remove_table(const char *table_name)
{
    field_table_t *table;

    if (!g_shared_hash_map || !table_name) {
        return -1;
    }

    pthread_mutex_lock(&g_hash_map_mutex);
    
    HASH_FIND_STR(g_shared_hash_map->tables, table_name, table);
    if (!table) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }

    HASH_DEL(g_shared_hash_map->tables, table);
    destroy_field_table(table);
    g_shared_hash_map->size--;
    
    pthread_mutex_unlock(&g_hash_map_mutex);

    return 0;
}

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, linx_field_type_t type)
{
    field_table_t *table;
    field_info_t *existing_field, *new_field;

    if (!g_shared_hash_map || !table_name || !field_name) {
        return -1;
    }

    pthread_mutex_lock(&g_hash_map_mutex);
    
    HASH_FIND_STR(g_shared_hash_map->tables, table_name, table);
    if (!table) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }

    HASH_FIND_STR(table->fields, field_name, existing_field);
    if (existing_field != NULL) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }

    new_field = malloc(sizeof(field_info_t));
    if (new_field == NULL) {
        pthread_mutex_unlock(&g_hash_map_mutex);
        return -1;
    }

    new_field->key = strdup(field_name);
    new_field->offset = offset;
    new_field->type = type;
    new_field->size = size;

    HASH_ADD_STR(table->fields, key, new_field);
    
    pthread_mutex_unlock(&g_hash_map_mutex);

    return 0;
}

int linx_hash_map_add_field_batch(const char *table_name, const field_mapping_t *mappings, size_t count)
{
    int ret;

    if (!g_shared_hash_map || !table_name || !mappings) {
        return -1;
    }

    ret = linx_hash_map_create_table(table_name, NULL);
    if (ret) {
        return ret;
    }

    for (size_t i = 0; i < count; i++) {
        ret = linx_hash_map_add_field(table_name, mappings[i].field_name, mappings[i].offset, mappings[i].size, mappings[i].type);
        if (ret) {
            return ret;
        }
    }

    return ret;
}

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name)
{
    field_table_t *table;
    field_info_t *field;
    field_result_t result = {0};

    result.found = false;

    if (!g_shared_hash_map || !table_name || !field_name) {
        return result;
    }

    /* 读取共享字段映射，不需要加锁（只读操作） */
    HASH_FIND_STR(g_shared_hash_map->tables, table_name, table);
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
    char *table_name, *field_name, *arg;
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

    result = linx_hash_map_get_field(table_name, field_name);

    arg = strtok(NULL, ".");
    if (arg == NULL) {
        result.arg = NULL;
    } else {
        result.arg = strdup(arg);
    }

    result.event_type = &(linx_event_rich_get()->num);

    return result;
}

void *linx_hash_map_get_value_ptr(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    /* 从线程特定存储获取base_addr */
    base_addr = linx_thread_base_addr_get(field->table_name);
    if (base_addr == NULL) {
        return NULL;
    }

    if (field->arg) {
        char *endptr;
        long index = strtol(field->arg, &endptr, 10);

        if (field->type == LINX_FIELD_TYPE_STRUCT) {
            /* 如果不是纯数字，则通过名称查找下标 */
            if (*endptr != '\0') {
                for (index = 0; index < g_linx_event_table[*field->event_type].nparams; index++) {
                    if (strcmp(g_linx_event_table[*field->event_type].params[index].name, field->arg) == 0) {
                        break;
                    }
                }
            }

            if (index >= g_linx_event_table[*field->event_type].nparams) {
                field->arg_index = -1;
                return NULL;
            }

            ptr = (void *)((char *)base_addr + field->offset + index * FIELD_STRUCT_SIZE);
            *type = g_linx_event_table[*field->event_type].params[index].type;
            field->arg_index = index;
        } else {
            if (*endptr != '\0') {

            }

            ptr = (void *)((char *)base_addr + field->offset + index * sizeof(void *));
            *type = field->type;
            field->arg_index = index;
        }
    } else {
        ptr = (void *)((char *)base_addr + field->offset);
        *type = field->type;
    }

    return ptr;
}

int linx_hash_map_update_table_base(const char *table_name, void *base_addr)
{
    if (!table_name) {
        return -1;
    }

    /* 直接设置到线程特定存储 */
    return linx_thread_base_addr_set(table_name, base_addr);
}

int linx_hash_map_update_tables_base(field_update_table_t *tables, size_t num_tables)
{
    if (!tables) {
        return -1;
    }

    /* 使用批量设置函数 */
    return linx_thread_base_addr_set_batch(tables, num_tables);
}

void *linx_hash_map_get_table_base(const char *table_name)
{
    if (!table_name) {
        return NULL;
    }

    /* 从线程特定存储获取base_addr */
    return linx_thread_base_addr_get(table_name);
}

int linx_hash_map_list_tables(char ***table_names, size_t *num_tables)
{
    field_table_t *current, *tmp;
    char **names;
    size_t table_count = 0;
    size_t index = 0;

    if (!g_shared_hash_map || !table_names || !num_tables) {
        return -1;
    }

    /* 使用共享表进行计数，不需要加锁（只读操作） */
    HASH_ITER(hh, g_shared_hash_map->tables, current, tmp) {
        table_count++;
    }

    if (table_count == 0) {
        *table_names = NULL;
        *num_tables = 0;
        return 0;
    }

    names = malloc(sizeof(char *) * table_count);
    if (!names) {
        return -1;
    }

    HASH_ITER(hh, g_shared_hash_map->tables, current, tmp) {
        names[index] = strdup(current->table_name);
        if (!names[index]) {
            for (size_t i = 0; i < index; i++) {
                free(names[i]);
                names[i] = NULL;
            }

            free(names);
            names = NULL;
            return -1;
        }

        index++;
    }

    *table_names = names;
    *num_tables = table_count;
    return 0;
}

void linx_hash_map_free_table_list(char **table_names, size_t num_tables)
{
    if (!table_names) {
        return;
    }

    for (size_t i = 0; i < num_tables; i++) {
        free(table_names[i]);
        table_names[i] = NULL;
    }

    free(table_names);
    table_names = NULL;
}
