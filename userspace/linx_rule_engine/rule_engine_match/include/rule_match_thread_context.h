#ifndef __RULE_MATCH_THREAD_CONTEXT_H__
#define __RULE_MATCH_THREAD_CONTEXT_H__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "linx_event_get.h"
#include "linx_event_rich.h"

/* 线程本地事件上下文 */
typedef struct {
    event_t evt;                    /* 事件数据的副本 */
    void *fd_cache;                 /* fd信息缓存 */
    void *proc_cache;               /* 进程信息缓存 */
    void *user_cache;               /* 用户信息缓存 */
    void *group_cache;              /* 组信息缓存 */
    field_update_table_t *tables;   /* 字段更新表 */
    size_t table_count;             /* 表数量 */
} thread_event_context_t;

/* 线程池配置 */
typedef struct {
    int num_threads;                /* 线程数量 */
    int queue_size;                 /* 任务队列大小 */
} thread_pool_config_t;

/* 规则匹配任务 */
typedef struct {
    linx_event_t *event;           /* 原始事件 */
    int64_t fd;                    /* 文件描述符 */
    size_t rule_start;             /* 起始规则索引 */
    size_t rule_end;               /* 结束规则索引 */
    bool *match_result;            /* 匹配结果 */
    pthread_mutex_t *result_mutex; /* 结果互斥锁 */
} rule_match_task_t;

/* 初始化线程池 */
int linx_rule_match_thread_pool_init(thread_pool_config_t *config);

/* 销毁线程池 */
void linx_rule_match_thread_pool_destroy(void);

/* 创建线程本地事件上下文 */
thread_event_context_t *linx_create_thread_event_context(linx_event_t *event, int64_t fd);

/* 销毁线程本地事件上下文 */
void linx_destroy_thread_event_context(thread_event_context_t *context);

/* 在指定的事件上下文中匹配规则 */
bool linx_match_rules_in_context(thread_event_context_t *context, size_t start_idx, size_t end_idx);

/* 多线程规则匹配 */
bool linx_rule_set_match_rule_mt(linx_event_t *event, int64_t fd);

#endif /* __RULE_MATCH_THREAD_CONTEXT_H__ */