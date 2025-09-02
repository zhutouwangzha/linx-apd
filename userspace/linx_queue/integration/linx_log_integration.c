/**
 * @file linx_log_integration.c
 * @brief 展示如何将通用队列集成到现有的日志模块中
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "linx_log_integration.h"

/* 全局日志实例 */
static linx_log_optimized_t *g_log_instance = NULL;

/* 日志级别字符串 */
static char *log_level_strings[LINX_LOG_LEVEL_MAX] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

/**
 * @brief 将日志级别字符串转换为枚举值
 */
static linx_log_level_t log_level_str_to_enum(const char *level_str)
{
    if (!level_str) return LINX_LOG_LEVEL_MAX;
    
    for (int i = 0; i < LINX_LOG_LEVEL_MAX; i++) {
        if (strcmp(level_str, log_level_strings[i]) == 0) {
            return (linx_log_level_t)i;
        }
    }
    
    return LINX_LOG_LEVEL_MAX;
}

/**
 * @brief 释放日志消息
 */
static void free_log_message(void *data)
{
    linx_log_message_t *msg = (linx_log_message_t *)data;
    if (msg) {
        free(msg->message);
        free(msg);
    }
}

/**
 * @brief 优化后的日志处理线程
 * 
 * 🔥 关键改进：
 * 1. 使用通用队列的超时pop，消除忙等待
 * 2. 快速响应停止信号
 * 3. 批量处理提升性能
 */
static void *optimized_log_thread(void *arg, int *should_stop)
{
    (void)arg;
    linx_log_message_t *msg;
    linx_queue_result_t result;
    char time_buf[64];
    struct tm *tm_info;
    int batch_count = 0;
    const int BATCH_SIZE = 10;  // 批量处理大小
    
    while (!*should_stop) {
        /* 🔥 使用通用队列的超时pop，100ms超时 */
        result = linx_queue_pop_timeout(g_log_instance->message_queue, 
                                       (void **)&msg, 100);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            /* 超时，检查停止条件并继续 */
            if (batch_count > 0) {
                /* 刷新缓冲区 */
                if (g_log_instance->log_file) {
                    fflush(g_log_instance->log_file);
                }
                batch_count = 0;
            }
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            /* 被中断，准备退出 */
            break;
        } else if (result == LINX_QUEUE_EMPTY) {
            /* 队列为空，继续等待 */
            continue;
        } else if (result != LINX_QUEUE_OK) {
            /* 其他错误 */
            g_log_instance->error_messages++;
            continue;
        }
        
        /* 检查立即停止条件 */
        if (*should_stop == 2) {
            /* 立即停止，释放当前消息 */
            free_log_message(msg);
            break;
        }
        
        /* 处理日志消息 */
        tm_info = localtime(&msg->tv.tv_sec);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        if (g_log_instance->log_file) {
            fprintf(g_log_instance->log_file, "[%s.%03ld] [%s] %s\n",
                    time_buf, msg->tv.tv_usec / 1000,
                    log_level_strings[msg->level], msg->message);
            
            batch_count++;
            
            /* 🔥 批量刷新，提升性能 */
            if (batch_count >= BATCH_SIZE) {
                fflush(g_log_instance->log_file);
                batch_count = 0;
            }
        }
        
        /* 释放消息 */
        free_log_message(msg);
        
        /* 优雅停止检查 */
        if (*should_stop == 1) {
            /* 优雅停止：检查队列是否为空 */
            if (linx_queue_is_empty(g_log_instance->message_queue)) {
                break;
            }
        }
    }
    
    /* 最终刷新 */
    if (g_log_instance->log_file && batch_count > 0) {
        fflush(g_log_instance->log_file);
    }
    
    return NULL;
}

/**
 * @brief 初始化优化后的日志系统
 */
int linx_log_optimized_init(const char *log_file, const char *log_level, 
                           const linx_queue_config_t *queue_config)
{
    int ret = 0;
    linx_log_level_t level = log_level_str_to_enum(log_level);
    linx_queue_config_t default_config;
    
    /* 验证参数 */
    if (level >= LINX_LOG_LEVEL_MAX) {
        return -1;
    }
    
    /* 检查是否已初始化 */
    if (g_log_instance) {
        return 0;
    }
    
    /* 分配日志实例 */
    g_log_instance = calloc(1, sizeof(linx_log_optimized_t));
    if (!g_log_instance) {
        return -1;
    }
    
    /* 设置日志级别 */
    g_log_instance->level = level;
    
    /* 设置日志输出目标 */
    if (log_file && strcmp(log_file, "stderr") != 0) {
        g_log_instance->log_file = fopen(log_file, "a");
        if (!g_log_instance->log_file) {
            free(g_log_instance);
            g_log_instance = NULL;
            return -1;
        }
    } else {
        g_log_instance->log_file = stderr;
    }
    
    /* 🔥 创建通用队列 */
    if (queue_config) {
        g_log_instance->message_queue = linx_queue_create(queue_config);
    } else {
        /* 使用优化的默认配置 */
        default_config = (linx_queue_config_t){
            .initial_capacity = 1024,    // 较大的初始容量
            .max_capacity = 0,           // 无限制
            .auto_resize = true,
            .resize_factor = 2,
            .thread_safe = true,
            .use_condition = true        // 启用条件变量
        };
        g_log_instance->message_queue = linx_queue_create(&default_config);
    }
    
    if (!g_log_instance->message_queue) {
        if (g_log_instance->log_file != stderr) {
            fclose(g_log_instance->log_file);
        }
        free(g_log_instance);
        g_log_instance = NULL;
        return -1;
    }
    
    /* 创建线程池 */
    g_log_instance->thread_pool = linx_thread_pool_create(1);
    if (!g_log_instance->thread_pool) {
        linx_queue_destroy(g_log_instance->message_queue, free_log_message);
        if (g_log_instance->log_file != stderr) {
            fclose(g_log_instance->log_file);
        }
        free(g_log_instance);
        g_log_instance = NULL;
        return -1;
    }
    
    /* 启动日志处理线程 */
    ret = linx_thread_pool_add_task(g_log_instance->thread_pool, 
                                   optimized_log_thread, NULL);
    if (ret) {
        linx_log_optimized_deinit();
        return -1;
    }
    
    return 0;
}

/**
 * @brief 关闭优化后的日志系统
 */
void linx_log_optimized_deinit(void)
{
    if (!g_log_instance) return;
    
    /* 🔥 中断队列上的等待操作 */
    if (g_log_instance->message_queue) {
        linx_queue_interrupt_all(g_log_instance->message_queue);
    }
    
    /* 销毁线程池（优雅关闭） */
    if (g_log_instance->thread_pool) {
        linx_thread_pool_destroy(g_log_instance->thread_pool, 1);
        g_log_instance->thread_pool = NULL;
    }
    
    /* 销毁队列 */
    if (g_log_instance->message_queue) {
        linx_queue_destroy(g_log_instance->message_queue, free_log_message);
        g_log_instance->message_queue = NULL;
    }
    
    /* 关闭日志文件 */
    if (g_log_instance->log_file && g_log_instance->log_file != stderr) {
        fclose(g_log_instance->log_file);
    }
    
    /* 释放实例 */
    free(g_log_instance);
    g_log_instance = NULL;
}

/**
 * @brief 记录日志（优化版本）
 */
void linx_log_optimized(linx_log_level_t level, const char *file, int line, 
                       const char *format, ...)
{
    va_list args;
    
    if (level < g_log_instance->level) {
        return;
    }
    
    va_start(args, format);
    linx_log_optimized_v(level, file, line, format, args);
    va_end(args);
}

/**
 * @brief 记录日志（可变参数版本）
 */
void linx_log_optimized_v(linx_log_level_t level, const char *file, int line, 
                         const char *format, va_list args)
{
    linx_log_message_t *msg;
    char msg_buf[1024];
    int len;
    linx_queue_result_t result;
    
    if (!g_log_instance || level < g_log_instance->level) {
        return;
    }
    
    /* 构建日志消息 */
    len = snprintf(msg_buf, sizeof(msg_buf), "[%s:%d]: ", file, line);
    if (len < 0 || len >= (int)sizeof(msg_buf)) {
        return;
    }
    
    len = vsnprintf(msg_buf + len, sizeof(msg_buf) - len, format, args);
    if (len < 0) {
        return;
    }
    
    /* 分配消息结构体 */
    msg = malloc(sizeof(linx_log_message_t));
    if (!msg) {
        g_log_instance->dropped_messages++;
        return;
    }
    
    /* 填充消息 */
    gettimeofday(&msg->tv, NULL);
    msg->level = level;
    msg->message = strdup(msg_buf);
    if (!msg->message) {
        free(msg);
        g_log_instance->dropped_messages++;
        return;
    }
    
    /* 🔥 使用通用队列入队 */
    result = linx_queue_push(g_log_instance->message_queue, msg);
    if (result != LINX_QUEUE_OK) {
        /* 入队失败，释放消息 */
        free_log_message(msg);
        g_log_instance->dropped_messages++;
        
        if (result == LINX_QUEUE_FULL) {
            /* 队列满，记录错误 */
            fprintf(stderr, "[LOG_ERROR] Message queue full, message dropped\n");
        }
    } else {
        g_log_instance->total_messages++;
    }
}

/**
 * @brief 获取日志系统统计信息
 */
int linx_log_get_statistics(linx_log_statistics_t *stats)
{
    if (!g_log_instance || !stats) {
        return -1;
    }
    
    stats->total_messages = g_log_instance->total_messages;
    stats->dropped_messages = g_log_instance->dropped_messages;
    stats->error_messages = g_log_instance->error_messages;
    
    /* 获取队列统计信息 */
    return (linx_queue_get_stats(g_log_instance->message_queue, 
                                &stats->queue_stats) == LINX_QUEUE_OK) ? 0 : -1;
}

/**
 * @brief 优雅关闭日志系统
 */
int linx_log_graceful_shutdown(int timeout_ms)
{
    time_t start_time, current_time;
    
    if (!g_log_instance) {
        return 0;
    }
    
    start_time = time(NULL);
    
    /* 等待队列变空 */
    while (!linx_queue_is_empty(g_log_instance->message_queue)) {
        if (timeout_ms > 0) {
            current_time = time(NULL);
            if ((current_time - start_time) * 1000 >= timeout_ms) {
                return -1; // 超时
            }
        }
        
        usleep(10000); // 10ms
    }
    
    /* 正常关闭 */
    linx_log_optimized_deinit();
    return 0;
}