#ifndef __LINX_EVENT_PROCESSOR_CONFIG_H__
#define __LINX_EVENT_PROCESSOR_CONFIG_H__ 

#include <stdbool.h>

typedef struct {
    uint32_t thread_cound;
    bool cpu_affinity;
    int priority;
} linx_thread_pool_config_t;

typedef struct {
    /* 核心配置 */
    uint32_t fetcher_thread_count;
    uint32_t matcher_thread_count;

    /* 子模块配置 */
    linx_thread_pool_config_t fetcher_pool_config;
    linx_thread_pool_config_t matcher_pool_config;
} linx_event_processor_config_t;

#endif /* __LINX_EVENT_PROCESSOR_CONFIG_H__ */
