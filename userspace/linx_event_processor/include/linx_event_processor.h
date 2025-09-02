#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>

#include "linx_thread_pool.h"
#include "linx_event_processor_config.h"
#include "linx_event_processor_define.h"
#include "linx_event.h"

typedef struct linx_event_processor_s {
    /* 配置 */
    linx_event_processor_config_t config;

    /* 线程池 */
    linx_thread_pool_t *fetcher_pool;
    linx_thread_pool_t *matcher_pool;
} linx_event_processor_t;

int linx_event_processor_init(linx_event_processor_config_t *config);

void linx_event_processor_deinit(void);

int linx_event_processor_start(void);

int linx_event_processor_stop(void);

/* 处理单个事件（用于与现有事件循环集成） */
int linx_event_processor_process_event(linx_event_t *event, int64_t fd);

/* 获取全局事件处理器实例 */
linx_event_processor_t *linx_event_processor_get(void);

#endif /* __LINX_EVENT_PROCESSOR_H__ */
