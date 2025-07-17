#ifndef __LINX_EVENT_QUEUE_H__
#define __LINX_EVENT_QUEUE_H__ 

#include <stdint.h>

int linx_event_queue_init(uint64_t capacity);

int linx_event_queue_push(void);

void linx_event_queue_free(void);

#endif /* __LINX_EVENT_QUEUE_H__ */
