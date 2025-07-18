#ifndef __LINX_EVENT_PROCESSOR_V2_H__
#define __LINX_EVENT_PROCESSOR_V2_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "linx_thread_pool.h"

/* 事件处理器配置 */
typedef struct {
    int event_fetcher_pool_size;     /* 事件获取线程池大小 */
    int rule_matcher_pool_size;      /* 规则匹配线程池大小 */
    int event_queue_size;            /* 事件队列大小 */
    int batch_size;                  /* 批处理大小 */
} linx_event_processor_v2_config_t;

/* 事件数据结构 */
typedef struct {
    void *raw_event;                 /* 原始事件数据 */
    void *enriched_event;            /* 丰富后的事件数据 */
    uint64_t timestamp;              /* 时间戳 */
    int event_id;                    /* 事件ID */
    int priority;                    /* 事件优先级 */
} linx_event_data_v2_t;

/* 任务类型 */
typedef enum {
    TASK_TYPE_EVENT_FETCH,           /* 事件获取任务 */
    TASK_TYPE_RULE_MATCH,            /* 规则匹配任务 */
    TASK_TYPE_BATCH_PROCESS,         /* 批处理任务 */
    TASK_TYPE_MAX
} linx_task_type_t;

/* 任务参数结构 */
typedef struct {
    linx_task_type_t type;           /* 任务类型 */
    union {
        struct {
            int fetch_count;         /* 要获取的事件数量 */
        } fetch_task;
        
        struct {
            linx_event_data_v2_t *event;  /* 要匹配的事件 */
        } match_task;
        
        struct {
            linx_event_data_v2_t *events;  /* 事件批次 */
            int count;                      /* 事件数量 */
        } batch_task;
    } params;
} linx_task_arg_v2_t;

/* 事件队列结构 (基于现有线程池任务队列概念) */
typedef struct {
    linx_event_data_v2_t *events;   /* 事件数组 */
    int capacity;                    /* 队列容量 */
    int head;                        /* 队列头 */
    int tail;                        /* 队列尾 */
    int count;                       /* 当前事件数量 */
    pthread_mutex_t mutex;           /* 队列互斥锁 */
    pthread_cond_t not_empty;        /* 非空条件变量 */
    pthread_cond_t not_full;         /* 非满条件变量 */
} linx_event_queue_v2_t;

/* 事件处理器结构 */
typedef struct {
    linx_event_processor_v2_config_t config;
    
    /* 线程池 */
    linx_thread_pool_t *fetcher_pool;    /* 事件获取线程池 */
    linx_thread_pool_t *matcher_pool;    /* 规则匹配线程池 */
    
    /* 事件队列 */
    linx_event_queue_v2_t *event_queue;  /* 事件队列 */
    
    /* 控制标志 */
    volatile bool running;           /* 运行标志 */
    volatile bool shutdown;          /* 关闭标志 */
    
    /* 统计信息 */
    uint64_t total_events;           /* 总事件数 */
    uint64_t processed_events;       /* 已处理事件数 */
    uint64_t matched_events;         /* 匹配的事件数 */
    uint64_t failed_events;          /* 失败的事件数 */
    pthread_mutex_t stats_mutex;     /* 统计信息互斥锁 */
    
    /* 性能监控 */
    uint64_t last_stats_time;        /* 上次统计时间 */
    uint64_t fetch_tasks_submitted;  /* 提交的获取任务数 */
    uint64_t match_tasks_submitted;  /* 提交的匹配任务数 */
} linx_event_processor_v2_t;

/* 初始化和清理函数 */
int linx_event_processor_v2_init(linx_event_processor_v2_config_t *config);
void linx_event_processor_v2_cleanup(void);

/* 启动和停止函数 */
int linx_event_processor_v2_start(void);
int linx_event_processor_v2_stop(void);

/* 队列操作函数 */
int linx_event_queue_v2_create(linx_event_queue_v2_t **queue, int capacity);
void linx_event_queue_v2_destroy(linx_event_queue_v2_t *queue);
int linx_event_queue_v2_push(linx_event_queue_v2_t *queue, linx_event_data_v2_t *event);
int linx_event_queue_v2_pop(linx_event_queue_v2_t *queue, linx_event_data_v2_t *event);
int linx_event_queue_v2_pop_batch(linx_event_queue_v2_t *queue, linx_event_data_v2_t *events, int max_count);

/* 任务函数 */
void *linx_event_fetch_task(void *arg, int *should_stop);
void *linx_rule_match_task(void *arg, int *should_stop);
void *linx_batch_process_task(void *arg, int *should_stop);

/* 统计信息函数 */
void linx_event_processor_v2_get_stats(uint64_t *total, uint64_t *processed, uint64_t *matched, uint64_t *failed);
void linx_event_processor_v2_print_stats(void);

/* 辅助函数 */
int linx_event_processor_v2_submit_fetch_tasks(int count);
int linx_event_processor_v2_submit_match_task(linx_event_data_v2_t *event);
int linx_event_processor_v2_get_queue_status(int *queue_size, int *fetcher_active, int *matcher_active);

#endif /* __LINX_EVENT_PROCESSOR_V2_H__ */