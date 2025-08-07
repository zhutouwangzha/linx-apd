#ifndef __USER_STRUCT_H__
#define __USER_STRUCT_H__

#include <stdint.h>

typedef struct {
    uint32_t uid;
    char name[32];
    char homedir[256];
    char shell[32];
    int64_t loginuid;
    char loginname[32];
} user_t;

#endif /* __USER_STRUCT_H__ */
