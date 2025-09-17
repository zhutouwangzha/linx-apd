#ifndef __LINX_EVENT_RICH_MT_H__
#define __LINX_EVENT_RICH_MT_H__

#include <pthread.h>
#include "linx_event_rich.h"
#include "linx_thread_context.h"

/**
 * 多线程事件丰富化初始化
 * 必须在创建工作线程之前调用
 * @return 0 成功，-1 失败
 */
int linx_event_rich_mt_init(void);

/**
 * 多线程事件丰富化清理
 * 在所有工作线程结束后调用
 */
void linx_event_rich_mt_deinit(void);

/**
 * 为当前线程初始化事件丰富化上下文
 * 每个工作线程都必须调用此函数
 * @return 0 成功，-1 失败
 */
int linx_event_rich_thread_init(void);

/**
 * 清理当前线程的事件丰富化上下文
 * 每个工作线程结束前都必须调用此函数
 */
void linx_event_rich_thread_deinit(void);

/**
 * 多线程安全的事件丰富化处理
 * 每个线程使用自己的上下文处理事件
 * @param event 要处理的事件
 * @return 0 成功，-1 失败
 */
int linx_event_rich_mt(linx_event_t *event);

/**
 * 获取当前线程的事件结构
 * @return event_t指针，NULL表示未初始化
 */
event_t *linx_event_rich_mt_get(void);

/**
 * 线程工作函数示例
 * @param arg 线程参数
 * @return NULL
 */
void *linx_event_rich_worker_thread(void *arg);

/**
 * 线程池工作参数结构
 */
typedef struct {
    int thread_id;
    void *event_queue;      /* 事件队列，具体实现由用户定义 */
    void *user_data;        /* 用户自定义数据 */
    bool *stop_flag;        /* 停止标志 */
} linx_worker_args_t;

#endif /* __LINX_EVENT_RICH_MT_H__ */