#ifndef __LINX_EVENT_TABLE_H__
#define __LINX_EVENT_TABLE_H__ 

#include <stdint.h>

#include "linx_event_type.h"

typedef struct {
    const char *name;
    uint32_t enter_num;
    uint32_t exit_num;
} linx_event_table_t;

__attribute__((weak))
linx_event_table_t linx_event_table[LINX_EVENT_TYPE_MAX] = {
    [LINX_EVENT_TYPE_OPEN_E] = {
        .name = "open",
        .enter_num = 2,
        .exit_num = 1
    },
};

#endif /* __LINX_EVENT_TABLE_H__ */
