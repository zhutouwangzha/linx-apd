#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "linx_thread_pool.h"
#include "linx_event_processor_config.h"

/* 前向声明 */
typedef struct linx_ebpf_s linx_ebpf_t;
typedef struct linx_event_queue_s linx_event_queue_t;

/* 事件处理器统计信息 */
typedef struct {
    uint64_t total_events;              /* 总事件数 */
    uint64_t processed_events;          /* 已处理事件数 */
    uint64_t matched_events;            /* 匹配成功事件数 */
    uint64_t failed_events;             /* 处理失败事件数 */
    uint64_t fetch_tasks_submitted;     /* 提交的获取任务数 */
    uint64_t match_tasks_submitted;     /* 提交的匹配任务数 */
    
    int fetcher_active_threads;         /* 活跃的获取线程数 */
    int matcher_active_threads;         /* 活跃的匹配线程数 */
    int fetcher_queue_size;             /* 获取线程池队列大小 */
    int matcher_queue_size;             /* 匹配线程池队列大小 */
    
    bool running;                       /* 是否正在运行 */
} linx_event_processor_stats_t;

/* 事件处理器主结构体 */
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

/* 初始化和清理函数 */

/**
 * @brief 初始化事件处理器
 * 
 * @param config 配置参数，传入NULL使用默认配置
 * @return int 成功返回0，失败返回-1
 */
int linx_event_processor_init(linx_event_processor_config_t *config);

/**
 * @brief 销毁事件处理器
 */
void linx_event_processor_deinit(void);

/* 运行控制函数 */

/**
 * @brief 启动事件处理器
 * 
 * @param ebpf_manager eBPF管理器实例
 * @return int 成功返回0，失败返回-1
 */
int linx_event_processor_start(linx_ebpf_t *ebpf_manager);

/**
 * @brief 停止事件处理器
 * 
 * @return int 成功返回0，失败返回-1
 */
int linx_event_processor_stop(void);

/* 状态查询函数 */

/**
 * @brief 检查事件处理器是否正在运行
 * 
 * @return bool 正在运行返回true，否则返回false
 */
bool linx_event_processor_is_running(void);

/**
 * @brief 获取事件处理器统计信息
 * 
 * @param stats 统计信息结构体指针
 */
void linx_event_processor_get_stats(linx_event_processor_stats_t *stats);

#endif /* __LINX_EVENT_PROCESSOR_H__ */
