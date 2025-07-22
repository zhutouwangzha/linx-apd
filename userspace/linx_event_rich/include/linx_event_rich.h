#ifndef __LINX_EVENT_RICH_H__
#define __LINX_EVENT_RICH_H__

#include <stdint.h>

#include "linx_event.h"

int linx_event_rich_init(void);

int linx_event_rich(linx_event_t *event);

int linx_event_rich_deinit(void);

#endif /* __LINX_EVENT_RICH_H__ */
