/**
 * @file linx_log_queue_example.c
 * @brief 演示如何在日志模块中使用通用队列
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "linx_queue.h"

/* 日志消息结构体 */
typedef struct {
    int level;
    struct timeval tv;
    char *message;
} log_message_t;

/* 全局日志队列 */
static linx_queue_t *g_log_queue = NULL;
static pthread_t g_log_thread;
static volatile bool g_log_running = false;

/* 日志级别字符串 */
static const char *level_strings[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

/**
 * @brief 释放日志消息
 */
static void free_log_message(void *data)
{
    log_message_t *msg = (log_message_t *)data;
    if (msg) {
        free(msg->message);
        free(msg);
    }
}

/**
 * @brief 日志处理线程
 */
static void *log_thread_func(void *arg)
{
    (void)arg;
    log_message_t *msg;
    linx_queue_result_t result;
    char time_buf[64];
    struct tm *tm_info;
    
    printf("[LOG_THREAD] Started\n");
    
    while (g_log_running) {
        /* 🔥 使用通用队列的超时pop，100ms超时 */
        result = linx_queue_pop_timeout(g_log_queue, (void **)&msg, 100);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            /* 超时，检查停止条件 */
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            /* 被中断，准备退出 */
            printf("[LOG_THREAD] Interrupted\n");
            break;
        } else if (result != LINX_QUEUE_OK) {
            /* 其他错误 */
            continue;
        }
        
        /* 处理日志消息 */
        tm_info = localtime(&msg->tv.tv_sec);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        printf("[%s.%03ld] [%s] %s\n",
               time_buf, msg->tv.tv_usec / 1000,
               level_strings[msg->level], msg->message);
        
        /* 释放消息 */
        free_log_message(msg);
    }
    
    printf("[LOG_THREAD] Stopped\n");
    return NULL;
}

/**
 * @brief 初始化日志系统
 */
int log_init(void)
{
    linx_queue_config_t config = LINX_QUEUE_DEFAULT_CONFIG();
    
    /* 创建日志队列 */
    g_log_queue = linx_queue_create(&config);
    if (!g_log_queue) {
        return -1;
    }
    
    /* 启动日志线程 */
    g_log_running = true;
    if (pthread_create(&g_log_thread, NULL, log_thread_func, NULL) != 0) {
        linx_queue_destroy(g_log_queue, free_log_message);
        g_log_queue = NULL;
        return -1;
    }
    
    return 0;
}

/**
 * @brief 关闭日志系统
 */
void log_shutdown(void)
{
    if (!g_log_queue) return;
    
    /* 停止日志线程 */
    g_log_running = false;
    
    /* 中断等待的线程 */
    linx_queue_interrupt_all(g_log_queue);
    
    /* 等待线程结束 */
    pthread_join(g_log_thread, NULL);
    
    /* 销毁队列 */
    linx_queue_destroy(g_log_queue, free_log_message);
    g_log_queue = NULL;
}

/**
 * @brief 记录日志
 */
void log_write(int level, const char *format, ...)
{
    log_message_t *msg;
    va_list args;
    char buffer[1024];
    
    if (!g_log_queue) return;
    
    /* 创建日志消息 */
    msg = malloc(sizeof(log_message_t));
    if (!msg) return;
    
    /* 格式化消息 */
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* 填充消息结构 */
    msg->level = level;
    gettimeofday(&msg->tv, NULL);
    msg->message = strdup(buffer);
    if (!msg->message) {
        free(msg);
        return;
    }
    
    /* 🔥 使用通用队列入队 */
    if (linx_queue_push(g_log_queue, msg) != LINX_QUEUE_OK) {
        /* 入队失败，释放消息 */
        free_log_message(msg);
    }
}

/**
 * @brief 获取日志统计信息
 */
void log_print_stats(void)
{
    linx_queue_stats_t stats;
    
    if (!g_log_queue) return;
    
    if (linx_queue_get_stats(g_log_queue, &stats) == LINX_QUEUE_OK) {
        printf("\n=== Log Queue Statistics ===\n");
        printf("Current size: %u\n", stats.current_size);
        printf("Current capacity: %u\n", stats.current_capacity);
        printf("Total pushed: %lu\n", stats.total_pushed);
        printf("Total popped: %lu\n", stats.total_popped);
        printf("Total timeouts: %lu\n", stats.total_timeouts);
        printf("Total resizes: %lu\n", stats.total_resizes);
        printf("============================\n\n");
    }
}

/* 示例程序 */
int main(void)
{
    int i;
    
    printf("=== Log Queue Example ===\n");
    
    /* 初始化日志系统 */
    if (log_init() != 0) {
        printf("Failed to initialize log system\n");
        return 1;
    }
    
    /* 写入一些日志 */
    for (i = 0; i < 10; i++) {
        log_write(i % 5, "Test message %d", i);
        usleep(10000); // 10ms
    }
    
    /* 等待一下让日志处理完 */
    sleep(1);
    
    /* 打印统计信息 */
    log_print_stats();
    
    /* 关闭日志系统 */
    log_shutdown();
    
    printf("=== Example Complete ===\n");
    return 0;
}