#ifndef __LINX_EVENT_RICH_H__
#define __LINX_EVENT_RICH_H__

#include "plugin/plugin_types.h"

#include "linx_event.h"
#include "plugin_field_idx.h"

typedef struct {
    void *value[PLUGIN_FIELDS_COMM];
} linx_event_rich_t;

int linx_event_rich_msg(linx_event_rich_t *rich, linx_event_t *event, ss_plugin_extract_field *field);

int linx_event_rich_init(linx_event_rich_t *rich);

void linx_event_rich_deinit(linx_event_rich_t *rich);

#endif /* __LINX_EVENT_RICH_H__ */
