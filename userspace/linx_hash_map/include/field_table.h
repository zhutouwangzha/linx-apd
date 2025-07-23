#ifndef __FIELD_TABLE_H__
#define __FIELD_TABLE_H__ 

#include <pthread.h>
#include "field_info.h"

/**
 * 表信息
*/
typedef struct {
    char *table_name;
    field_info_t *fields;
    void *base_addr;        /* 结构体的基地址 */
    pthread_rwlock_t rwlock; /* 保护基地址的读写锁 */
    UT_hash_handle hh;
} field_table_t;

#endif /* __FIELD_TABLE_H__ */
