#include <stddef.h>

#include "linx_event_get.h"
#include "linx_event_table.h"

void *linx_event_get_param(linx_event_t *event, uint32_t id)
{
    uint64_t size = 0;
    if (event == NULL || id >= g_linx_event_table[event->type].nparams) {
        return NULL;
    }

    for (uint32_t i = 0; i < id; ++i) {
        size += event->params_size[i];
    }

    return (void *)event + LINX_EVENT_HEADER_SIZE + size;
}