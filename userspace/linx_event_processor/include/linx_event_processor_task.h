#ifndef __LINX_EVENT_PROCESSOR_TASK_H__
#define __LINX_EVENT_PROCESSOR_TASK_H__ 

#include "linx_event_processor.h"
#include "linx_event.h"

typedef enum {
    LINX_TASK_TYPE_FETCH_EVENT,
    LINX_TASK_TYPE_MATCH_EVENT,
    LINX_TASK_TYPE_SHUTDOWN
} linx_event_processor_task_type_t;

typedef struct {
    linx_event_processor_task_type_t type;
    linx_event_processor_t *processor;
    linx_event_t *event;  /* Event to be processed by matcher threads */
    int worker_id;
} linx_event_processor_task_t;

#endif /* __LINX_EVENT_PROCESSOR_TASK_H__ */
