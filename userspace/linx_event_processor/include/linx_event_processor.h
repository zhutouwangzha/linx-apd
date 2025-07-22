#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>

#include "linx_thread_pool.h"
#include "linx_event_processor_config.h"

typedef struct {
    linx_event_processor_config_t config;

    linx_thread_pool_t *fetcher_pool;   /* 事件获取线程池 */
    linx_thread_pool_t *matcher_pool;   /* 规则匹配线程池 */

    linx_event_queue_t *event_queue;    /* 事件队列 */

    /* 控制标志 */
    volatile bool running;
    volatile bool shutdown;

    /* 统计信息 */
    uint64_t total_events;
    uint64_t processed_events;
    uint64_t matched_events;
    uint64_t failed_events;
    pthread_mutex_t status_mutex;

    /* 性能监控 */
    uint64_t last_stats_time;
    uint64_t fetch_tasks_submitted;
    uint64_t match_tasks_submitted;
} linx_event_processor_t;

int linx_event_processor_init(linx_event_processor_config_t *config);

void linx_event_processor_deinit(void);

#endif /* __LINX_EVENT_PROCESSOR_H__ */
