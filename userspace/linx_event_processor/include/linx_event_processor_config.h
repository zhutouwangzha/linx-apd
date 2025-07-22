#ifndef __LINX_EVENT_PROCESSOR_CONFIG_H__
#define __LINX_EVENT_PROCESSOR_CONFIG_H__ 

typedef struct {
    int event_fetcher_pool_size;
    int event_matcher_pool_size;
    int event_queue_size;
    int batch_size;                 /* 批处理的大小 */
} linx_event_processor_config_t;

#endif /* __LINX_EVENT_PROCESSOR_CONFIG_H__ */