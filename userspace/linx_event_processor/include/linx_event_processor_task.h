#ifndef __LINX_EVENT_PROCESSOR_TASK_H__
#define __LINX_EVENT_PROCESSOR_TASK_H__ 

#include <stdint.h>
#include "linx_event.h"

/* 前向声明避免循环依赖 */
struct linx_event_processor_s;

typedef enum {
    LINX_TASK_TYPE_FETCH_EVENT,
    LINX_TASK_TYPE_MATCH_EVENT,
    LINX_TASK_TYPE_SHUTDOWN
} linx_event_processor_task_type_t;

typedef struct {
    linx_event_processor_task_type_t type;
    struct linx_event_processor_s *processor;
    linx_event_t *event;
    int64_t fd;
    int worker_id;
} linx_event_processor_task_t;

#endif /* __LINX_EVENT_PROCESSOR_TASK_H__ */
