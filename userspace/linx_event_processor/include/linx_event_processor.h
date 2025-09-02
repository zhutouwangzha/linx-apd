#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "linx_thread_pool.h"
#include "linx_event_processor_config.h"
#include "linx_event_processor_define.h"
#include "linx_event.h"
#include "linx_event_get.h"
#include "event.h"
#include "linx_hash_map.h"

/* 线程事件上下文结构（从rule_match_mt移植） */
typedef struct {
    event_t evt;                       /* 事件数据副本 */
    field_update_table_t tables[5];    /* 字段更新表 */
    void *fd_info;                     /* fd信息 */
    void *proc_info;                   /* 进程信息 */
    void *user_info;                   /* 用户信息 */
    void *group_info;                  /* 组信息 */
} thread_event_context_t;

/* 规则匹配任务参数（从rule_match_mt移植并扩展） */
typedef struct {
    linx_event_t *event;               /* 事件数据 */
    int64_t fd;                        /* 文件描述符 */
    size_t rule_start;                 /* 起始规则索引 */
    size_t rule_end;                   /* 结束规则索引 */
    bool *match_result;                /* 匹配结果（共享） */
    pthread_mutex_t *result_mutex;     /* 结果互斥锁 */
    pthread_cond_t *complete_cond;     /* 完成条件变量 */
    int *completed_count;              /* 已完成任务数 */
} rule_match_task_arg_t;

typedef struct linx_event_processor_s {
    /* 配置 */
    linx_event_processor_config_t config;

    /* 线程池 */
    linx_thread_pool_t *fetcher_pool;
    linx_thread_pool_t *matcher_pool;
    
    /* 线程本地存储（从rule_match_mt移植） */
    pthread_key_t context_key;
    pthread_once_t key_once;
    bool initialized;
} linx_event_processor_t;

int linx_event_processor_init(linx_event_processor_config_t *config);

void linx_event_processor_deinit(void);

int linx_event_processor_start(void);

int linx_event_processor_stop(void);

/* 处理单个事件（用于与现有事件循环集成） - 返回匹配结果 */
bool linx_event_processor_process_event(linx_event_t *event, int64_t fd);

/* 获取全局事件处理器实例 */
linx_event_processor_t *linx_event_processor_get(void);

#endif /* __LINX_EVENT_PROCESSOR_H__ */
