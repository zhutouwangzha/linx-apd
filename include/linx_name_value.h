#ifndef __LINX_NAME_VALUE_H__
#define __LINX_NAME_VALUE_H__ 

#include <stdint.h>

typedef struct {
    const char *name;
    uint32_t value;
} linx_name_value_t;

extern const linx_name_value_t file_flags[];

#endif /* __LINX_NAME_VALUE_H__ */
