#ifndef __LINX_THREAD_CONTEXT_H__
#define __LINX_THREAD_CONTEXT_H__

#include <pthread.h>
#include "event.h"
#include "linx_hash_map.h"

/**
 * 简化的线程上下文结构，只包含必要的线程独立数据
 * base_addr 通过专门的 linx_thread_base_addr 机制管理
 */
typedef struct {
    /* 线程ID */
    pthread_t thread_id;
    
    /* 每个线程独立的事件结构 */
    event_t evt;
    
    /* 线程状态标识 */
    bool initialized;
} linx_thread_context_t;

/**
 * 初始化线程上下文系统
 * @return 0 成功，-1 失败
 */
int linx_thread_context_init(void);

/**
 * 清理线程上下文系统
 */
void linx_thread_context_deinit(void);

/**
 * 为当前线程创建上下文
 * @return 线程上下文指针，NULL表示失败
 */
linx_thread_context_t *linx_thread_context_create(void);

/**
 * 销毁当前线程的上下文
 */
void linx_thread_context_destroy(void);

/**
 * 获取当前线程的上下文
 * @return 线程上下文指针，NULL表示未初始化
 */
linx_thread_context_t *linx_thread_context_get(void);

/**
 * 获取当前线程的事件结构
 * @return event_t指针，NULL表示未初始化
 */
event_t *linx_thread_context_get_event(void);

/* 哈希表现在通过全局共享机制管理，不再需要线程特定的哈希表实例 */

#endif /* __LINX_THREAD_CONTEXT_H__ */