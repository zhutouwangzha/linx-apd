/**
 * @file linx_log_integration.h
 * @brief 展示如何将通用队列集成到现有的日志模块中
 */

#ifndef __LINX_LOG_INTEGRATION_H__
#define __LINX_LOG_INTEGRATION_H__

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include "linx_queue.h"
#include "linx_log_level.h"
#include "linx_log_message.h"
#include "linx_thread_pool.h"

/**
 * @brief 优化后的日志系统结构体
 */
typedef struct {
    linx_log_level_t level;
    FILE *log_file;
    
    /* 🔥 使用通用队列替代原有的数组队列 */
    linx_queue_t *message_queue;
    
    /* 线程管理 */
    linx_thread_pool_t *thread_pool;
    
    /* 统计信息 */
    uint64_t total_messages;
    uint64_t dropped_messages;
    uint64_t error_messages;
} linx_log_optimized_t;

/**
 * @brief 初始化优化后的日志系统
 * 
 * @param log_file 日志文件路径
 * @param log_level 日志级别字符串
 * @param queue_config 队列配置，NULL使用默认配置
 * @return 成功返回0，失败返回-1
 */
int linx_log_optimized_init(const char *log_file, const char *log_level, 
                           const linx_queue_config_t *queue_config);

/**
 * @brief 关闭优化后的日志系统
 */
void linx_log_optimized_deinit(void);

/**
 * @brief 记录日志（优化版本）
 */
void linx_log_optimized(linx_log_level_t level, const char *file, int line, 
                       const char *format, ...);

/**
 * @brief 记录日志（可变参数版本）
 */
void linx_log_optimized_v(linx_log_level_t level, const char *file, int line, 
                         const char *format, va_list args);

/**
 * @brief 获取日志系统统计信息
 */
typedef struct {
    uint64_t total_messages;
    uint64_t dropped_messages;
    uint64_t error_messages;
    linx_queue_stats_t queue_stats;
} linx_log_statistics_t;

int linx_log_get_statistics(linx_log_statistics_t *stats);

/**
 * @brief 优雅关闭日志系统
 * 
 * 该函数会等待所有排队的日志消息被处理完成后再关闭系统
 * 
 * @param timeout_ms 最大等待时间（毫秒），0表示立即关闭
 * @return 成功返回0，超时返回-1
 */
int linx_log_graceful_shutdown(int timeout_ms);

/* 便利宏定义（与原有API兼容） */
#define LINX_LOG_DEBUG_OPT(...)     linx_log_optimized(LINX_LOG_DEBUG,   __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_INFO_OPT(...)      linx_log_optimized(LINX_LOG_INFO,    __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_WARNING_OPT(...)   linx_log_optimized(LINX_LOG_WARNING, __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_ERROR_OPT(...)     linx_log_optimized(LINX_LOG_ERROR,   __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_FATAL_OPT(...)     linx_log_optimized(LINX_LOG_FATAL,   __FILE__, __LINE__, ##__VA_ARGS__)

#endif /* __LINX_LOG_INTEGRATION_H__ */