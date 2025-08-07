#ifndef __GROUP_STRUCT_H__
#define __GROUP_STRUCT_H__

#include <stdint.h>

typedef struct {
    uint32_t gid;
    char name[32];
} group_t;

#endif