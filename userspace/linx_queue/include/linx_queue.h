#ifndef __LINX_QUEUE_H__
#define __LINX_QUEUE_H__

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

/**
 * @brief 队列操作结果枚举
 */
typedef enum {
    LINX_QUEUE_OK = 0,           // 操作成功
    LINX_QUEUE_ERROR = -1,       // 通用错误
    LINX_QUEUE_EMPTY = -2,       // 队列为空
    LINX_QUEUE_FULL = -3,        // 队列已满
    LINX_QUEUE_TIMEOUT = -4,     // 操作超时
    LINX_QUEUE_INTERRUPTED = -5, // 操作被中断
    LINX_QUEUE_INVALID_ARG = -6  // 无效参数
} linx_queue_result_t;

/**
 * @brief 队列配置结构体
 */
typedef struct {
    uint32_t initial_capacity;   // 初始容量（0表示使用默认值16）
    uint32_t max_capacity;       // 最大容量（0表示无限制）
    bool auto_resize;            // 是否自动扩容
    uint32_t resize_factor;      // 扩容因子（默认2，即翻倍）
    bool thread_safe;            // 是否需要线程安全（使用互斥锁）
    bool use_condition;          // 是否使用条件变量（支持阻塞等待）
} linx_queue_config_t;

/**
 * @brief 通用队列结构体（对外不透明）
 */
typedef struct linx_queue linx_queue_t;

/**
 * @brief 队列统计信息
 */
typedef struct {
    uint32_t current_size;       // 当前元素数量
    uint32_t current_capacity;   // 当前容量
    uint64_t total_pushed;       // 总入队次数
    uint64_t total_popped;       // 总出队次数
    uint64_t total_timeouts;     // 超时次数
    uint64_t total_resizes;      // 扩容次数
} linx_queue_stats_t;

/**
 * @brief 释放函数类型定义
 * 
 * 用于在队列销毁时释放元素资源
 * 
 * @param data 要释放的数据指针
 */
typedef void (*linx_queue_free_func_t)(void *data);

/**
 * @brief 创建队列
 * 
 * @param config 队列配置，NULL表示使用默认配置
 * @return 成功返回队列指针，失败返回NULL
 */
linx_queue_t *linx_queue_create(const linx_queue_config_t *config);

/**
 * @brief 销毁队列
 * 
 * @param queue 队列指针
 * @param free_func 元素释放函数，NULL表示不释放元素
 */
void linx_queue_destroy(linx_queue_t *queue, linx_queue_free_func_t free_func);

/**
 * @brief 入队操作（非阻塞）
 * 
 * @param queue 队列指针
 * @param data 要入队的数据指针
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_push(linx_queue_t *queue, void *data);

/**
 * @brief 出队操作（非阻塞）
 * 
 * @param queue 队列指针
 * @param data 输出参数，存储出队的数据指针
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_pop(linx_queue_t *queue, void **data);

/**
 * @brief 出队操作（带超时的阻塞）
 * 
 * @param queue 队列指针
 * @param data 输出参数，存储出队的数据指针
 * @param timeout_ms 超时时间（毫秒），0表示非阻塞，-1表示永久等待
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_pop_timeout(linx_queue_t *queue, void **data, int timeout_ms);

/**
 * @brief 查看队头元素但不出队
 * 
 * @param queue 队列指针
 * @param data 输出参数，存储队头数据指针
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_peek(linx_queue_t *queue, void **data);

/**
 * @brief 获取队列大小
 * 
 * @param queue 队列指针
 * @return 队列中元素数量，失败返回-1
 */
int linx_queue_size(linx_queue_t *queue);

/**
 * @brief 检查队列是否为空
 * 
 * @param queue 队列指针
 * @return true表示为空，false表示非空
 */
bool linx_queue_is_empty(linx_queue_t *queue);

/**
 * @brief 检查队列是否已满
 * 
 * @param queue 队列指针
 * @return true表示已满，false表示未满
 */
bool linx_queue_is_full(linx_queue_t *queue);

/**
 * @brief 清空队列
 * 
 * @param queue 队列指针
 * @param free_func 元素释放函数，NULL表示不释放元素
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_clear(linx_queue_t *queue, linx_queue_free_func_t free_func);

/**
 * @brief 获取队列统计信息
 * 
 * @param queue 队列指针
 * @param stats 输出参数，存储统计信息
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_get_stats(linx_queue_t *queue, linx_queue_stats_t *stats);

/**
 * @brief 唤醒所有等待的线程
 * 
 * 用于优雅关闭时通知所有阻塞在pop操作上的线程
 * 
 * @param queue 队列指针
 * @return linx_queue_result_t 操作结果
 */
linx_queue_result_t linx_queue_interrupt_all(linx_queue_t *queue);

/**
 * @brief 获取默认配置
 * 
 * @param config 输出参数，填充默认配置
 */
void linx_queue_get_default_config(linx_queue_config_t *config);

/* 便利宏定义 */
#define LINX_QUEUE_DEFAULT_CONFIG() { \
    .initial_capacity = 16, \
    .max_capacity = 0, \
    .auto_resize = true, \
    .resize_factor = 2, \
    .thread_safe = true, \
    .use_condition = true \
}

#define LINX_QUEUE_SIMPLE_CONFIG(capacity) { \
    .initial_capacity = (capacity), \
    .max_capacity = (capacity), \
    .auto_resize = false, \
    .resize_factor = 0, \
    .thread_safe = true, \
    .use_condition = false \
}

#define LINX_QUEUE_LOCKFREE_CONFIG(capacity) { \
    .initial_capacity = (capacity), \
    .max_capacity = (capacity), \
    .auto_resize = false, \
    .resize_factor = 0, \
    .thread_safe = false, \
    .use_condition = false \
}

#endif /* __LINX_QUEUE_H__ */