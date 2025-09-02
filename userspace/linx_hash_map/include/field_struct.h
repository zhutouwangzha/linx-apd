#ifndef __FIELD_STRUCT_H__
#define __FIELD_STRUCT_H__

#include <stdint.h>

typedef struct {
    void *data;
    uint64_t size;
} field_struct_t;

#define FIELD_STRUCT_SIZE sizeof(field_struct_t)

#endif /* __FIELD_STRUCT_H__ */
