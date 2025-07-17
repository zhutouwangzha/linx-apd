#ifndef __FIELD_INFO_H__
#define __FIELD_INFO_H__ 

#include "uthash.h"

#include "field_type.h"

/**
 * 字段信息
*/
typedef struct {
    char *key;              /* 字段名 */
    size_t offset;          /* 在结构体内的偏移 */
    size_t size;            /* 该字段的大小 */
    field_type_t type;
    UT_hash_handle hh;
} field_info_t;

#endif /* __FIELD_INFO_H__ */
