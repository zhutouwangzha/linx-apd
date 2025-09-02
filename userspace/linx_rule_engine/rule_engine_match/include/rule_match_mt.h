#ifndef __RULE_MATCH_MT_H__
#define __RULE_MATCH_MT_H__

#include <stdbool.h>
#include "linx_event_get.h"
#include "linx_thread_pool.h"

/* 规则匹配多线程管理器 */
typedef struct {
    linx_thread_pool_t *thread_pool;    /* 线程池 */
    bool initialized;                   /* 是否已初始化 */
} rule_match_mt_manager_t;

/* 规则匹配任务参数 */
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

/* 初始化多线程规则匹配 */
int linx_rule_match_mt_init(int num_threads);

/* 清理多线程规则匹配 */
void linx_rule_match_mt_deinit(void);

/* 多线程规则匹配（使用现有的linx_thread_pool） */
bool linx_rule_set_match_rule_mt(linx_event_t *event, int64_t fd);

#endif /* __RULE_MATCH_MT_H__ */