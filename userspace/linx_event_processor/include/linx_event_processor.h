#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>

#include "linx_thread_pool.h"
#include "linx_event_processor_config.h"
#include "linx_event_processor_define.h"

typedef struct {
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

#endif /* __LINX_EVENT_PROCESSOR_H__ */
