#ifndef __LINX_EVENT_TABLE_H__
#define __LINX_EVENT_TABLE_H__ 

#include <stdint.h>

#include "linx_event_type.h"
#include "linx_field_type.h"

typedef struct {
    char name[32];
    linx_field_type_t type;
} linx_param_info_t;

typedef struct {
    char name[32];
    uint32_t nparams;
    linx_param_info_t params[32];
} linx_event_table_t;

extern const linx_event_table_t g_linx_event_table[LINX_EVENT_TYPE_MAX];

#endif /* __LINX_EVENT_TABLE_H__ */
