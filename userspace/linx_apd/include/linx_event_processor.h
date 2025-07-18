#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

/* 事件处理器配置 */
typedef struct {
    int event_fetcher_threads;    /* 事件获取线程数 */
    int rule_matcher_threads;     /* 规则匹配线程数 */
    int event_queue_size;         /* 事件队列大小 */
    int enriched_queue_size;      /* 丰富事件队列大小 */
} linx_event_processor_config_t;

/* 事件数据结构 */
typedef struct {
    void *raw_event;              /* 原始事件数据 */
    void *enriched_event;         /* 丰富后的事件数据 */
    uint64_t timestamp;           /* 时间戳 */
    int event_id;                 /* 事件ID */
} linx_event_data_t;

/* 事件队列结构 */
typedef struct {
    linx_event_data_t *events;    /* 事件数组 */
    int capacity;                 /* 队列容量 */
    int head;                     /* 队列头 */
    int tail;                     /* 队列尾 */
    int count;                    /* 当前事件数量 */
    pthread_mutex_t mutex;        /* 队列互斥锁 */
    pthread_cond_t not_empty;     /* 非空条件变量 */
    pthread_cond_t not_full;      /* 非满条件变量 */
} linx_event_queue_t;

/* 事件处理器结构 */
typedef struct {
    linx_event_processor_config_t config;
    
    /* 线程相关 */
    pthread_t *fetcher_threads;   /* 事件获取线程数组 */
    pthread_t *matcher_threads;   /* 规则匹配线程数组 */
    
    /* 队列相关 */
    linx_event_queue_t *raw_queue;      /* 原始事件队列 */
    linx_event_queue_t *enriched_queue; /* 丰富事件队列 */
    
    /* 控制标志 */
    volatile bool running;        /* 运行标志 */
    volatile bool shutdown;       /* 关闭标志 */
    
    /* 统计信息 */
    uint64_t total_events;        /* 总事件数 */
    uint64_t processed_events;    /* 已处理事件数 */
    uint64_t matched_events;      /* 匹配的事件数 */
    pthread_mutex_t stats_mutex;  /* 统计信息互斥锁 */
} linx_event_processor_t;

/* 初始化和清理函数 */
int linx_event_processor_init(linx_event_processor_config_t *config);
void linx_event_processor_cleanup(void);

/* 启动和停止函数 */
int linx_event_processor_start(void);
int linx_event_processor_stop(void);

/* 队列操作函数 */
int linx_event_queue_create(linx_event_queue_t **queue, int capacity);
void linx_event_queue_destroy(linx_event_queue_t *queue);
int linx_event_queue_push_event(linx_event_queue_t *queue, linx_event_data_t *event);
int linx_event_queue_pop_event(linx_event_queue_t *queue, linx_event_data_t *event);

/* 统计信息函数 */
void linx_event_processor_get_stats(uint64_t *total, uint64_t *processed, uint64_t *matched);

/* 工作线程函数 */
void *linx_event_fetcher_thread(void *arg);
void *linx_rule_matcher_thread(void *arg);

#endif /* __LINX_EVENT_PROCESSOR_H__ */