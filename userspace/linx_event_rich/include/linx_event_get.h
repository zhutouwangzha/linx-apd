#ifndef __LINX_EVENT_GET_H__
#define __LINX_EVENT_GET_H__ 

#include <stdint.h>

#include "linx_event.h"

void *linx_event_get_param(linx_event_t *event, uint32_t id);

#endif /* __LINX_EVENT_GET_H__ */
