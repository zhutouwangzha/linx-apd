#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "linx_hash_map.h"
#include "linx_log.h"

/* 全局互斥锁保护hash map操作 */
static pthread_rwlock_t g_hash_map_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * 方案2：使用读写锁保护全局状态
 * 
 * 优点：
 * - 实现简单，修改量小
 * - 多个线程可以并发读取基地址
 * - 保证更新操作的原子性
 * 
 * 缺点：
 * - 更新操作会阻塞所有读取操作
 * - 性能可能不如线程本地存储方案
 */

int linx_hash_map_update_tables_base_mutex(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;
    
    if (!tables) {
        return -1;
    }
    
    /* 获取写锁，确保更新操作的原子性 */
    pthread_rwlock_wrlock(&g_hash_map_rwlock);
    
    for (size_t i = 0; i < num_tables; i++) {
        ret = linx_hash_map_update_table_base(tables[i].table_name, tables[i].base_addr);
        if (ret) {
            LINX_LOG_WARNING("update %zu[%s] hash map table failed!", i, tables[i].table_name);
            break;
        }
    }
    
    pthread_rwlock_unlock(&g_hash_map_rwlock);
    
    return ret;
}

void *linx_hash_map_get_table_base_mutex(const char *table_name)
{
    void *base_addr;
    
    if (!table_name) {
        return NULL;
    }
    
    /* 获取读锁，允许并发读取 */
    pthread_rwlock_rdlock(&g_hash_map_rwlock);
    base_addr = linx_hash_map_get_table_base(table_name);
    pthread_rwlock_unlock(&g_hash_map_rwlock);
    
    return base_addr;
}

void *linx_hash_map_get_value_ptr_mutex(field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    /* 使用互斥锁保护的基地址获取 */
    base_addr = linx_hash_map_get_table_base_mutex(field->table_name);
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