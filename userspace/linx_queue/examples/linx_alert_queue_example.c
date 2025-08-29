/**
 * @file linx_alert_queue_example.c
 * @brief 演示如何在告警模块中使用通用队列
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "linx_queue.h"

/* 告警类型枚举 */
typedef enum {
    ALERT_TYPE_INFO,
    ALERT_TYPE_WARNING,
    ALERT_TYPE_CRITICAL,
    ALERT_TYPE_EMERGENCY
} alert_type_t;

/* 告警目标枚举 */
typedef enum {
    ALERT_TARGET_FILE,
    ALERT_TARGET_SYSLOG,
    ALERT_TARGET_HTTP,
    ALERT_TARGET_EMAIL
} alert_target_t;

/* 告警消息结构体 */
typedef struct {
    alert_type_t type;
    alert_target_t target;
    char *rule_name;
    char *message;
    time_t timestamp;
    int priority;
} alert_message_t;

/* 告警队列（按优先级分级） */
static linx_queue_t *g_high_priority_queue = NULL;  // 高优先级（紧急告警）
static linx_queue_t *g_normal_priority_queue = NULL; // 普通优先级
static pthread_t g_alert_threads[2]; // 两个处理线程
static volatile bool g_alert_running = false;

static const char *alert_type_strings[] = {
    "INFO", "WARNING", "CRITICAL", "EMERGENCY"
};

static const char *alert_target_strings[] = {
    "FILE", "SYSLOG", "HTTP", "EMAIL"
};

/**
 * @brief 释放告警消息
 */
static void free_alert_message(void *data)
{
    alert_message_t *msg = (alert_message_t *)data;
    if (msg) {
        free(msg->rule_name);
        free(msg->message);
        free(msg);
    }
}

/**
 * @brief 模拟发送告警到不同目标
 */
static void send_alert_to_target(alert_message_t *msg)
{
    char time_str[64];
    struct tm *tm_info = localtime(&msg->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    switch (msg->target) {
        case ALERT_TARGET_FILE:
            printf("[FILE] [%s] [%s] Rule: %s, Message: %s\n",
                   time_str, alert_type_strings[msg->type], 
                   msg->rule_name, msg->message);
            break;
            
        case ALERT_TARGET_SYSLOG:
            printf("[SYSLOG] [%s] [%s] Rule: %s, Message: %s\n",
                   time_str, alert_type_strings[msg->type], 
                   msg->rule_name, msg->message);
            break;
            
        case ALERT_TARGET_HTTP:
            printf("[HTTP] [%s] [%s] Rule: %s, Message: %s\n",
                   time_str, alert_type_strings[msg->type], 
                   msg->rule_name, msg->message);
            /* 模拟HTTP发送延迟 */
            usleep(50000); // 50ms
            break;
            
        case ALERT_TARGET_EMAIL:
            printf("[EMAIL] [%s] [%s] Rule: %s, Message: %s\n",
                   time_str, alert_type_strings[msg->type], 
                   msg->rule_name, msg->message);
            /* 模拟邮件发送延迟 */
            usleep(100000); // 100ms
            break;
    }
}

/**
 * @brief 高优先级告警处理线程
 */
static void *high_priority_alert_thread(void *arg)
{
    (void)arg;
    alert_message_t *msg;
    linx_queue_result_t result;
    
    printf("[HIGH_PRIORITY_THREAD] Started\n");
    
    while (g_alert_running) {
        /* 🔥 高优先级队列使用较短超时（50ms），确保快速响应 */
        result = linx_queue_pop_timeout(g_high_priority_queue, (void **)&msg, 50);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            break;
        } else if (result != LINX_QUEUE_OK) {
            continue;
        }
        
        printf("[HIGH_PRIORITY] Processing urgent alert\n");
        send_alert_to_target(msg);
        free_alert_message(msg);
    }
    
    printf("[HIGH_PRIORITY_THREAD] Stopped\n");
    return NULL;
}

/**
 * @brief 普通优先级告警处理线程
 */
static void *normal_priority_alert_thread(void *arg)
{
    (void)arg;
    alert_message_t *msg;
    linx_queue_result_t result;
    
    printf("[NORMAL_PRIORITY_THREAD] Started\n");
    
    while (g_alert_running) {
        /* 🔥 普通优先级队列使用较长超时（200ms） */
        result = linx_queue_pop_timeout(g_normal_priority_queue, (void **)&msg, 200);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            break;
        } else if (result != LINX_QUEUE_OK) {
            continue;
        }
        
        printf("[NORMAL_PRIORITY] Processing normal alert\n");
        send_alert_to_target(msg);
        free_alert_message(msg);
    }
    
    printf("[NORMAL_PRIORITY_THREAD] Stopped\n");
    return NULL;
}

/**
 * @brief 初始化告警系统
 */
int alert_init(void)
{
    linx_queue_config_t high_config = {
        .initial_capacity = 64,    // 高优先级队列较小
        .max_capacity = 1024,      // 限制最大容量
        .auto_resize = true,
        .resize_factor = 2,
        .thread_safe = true,
        .use_condition = true
    };
    
    linx_queue_config_t normal_config = {
        .initial_capacity = 256,   // 普通优先级队列较大
        .max_capacity = 0,         // 无限制
        .auto_resize = true,
        .resize_factor = 2,
        .thread_safe = true,
        .use_condition = true
    };
    
    /* 创建队列 */
    g_high_priority_queue = linx_queue_create(&high_config);
    g_normal_priority_queue = linx_queue_create(&normal_config);
    
    if (!g_high_priority_queue || !g_normal_priority_queue) {
        if (g_high_priority_queue) {
            linx_queue_destroy(g_high_priority_queue, free_alert_message);
        }
        if (g_normal_priority_queue) {
            linx_queue_destroy(g_normal_priority_queue, free_alert_message);
        }
        return -1;
    }
    
    /* 启动处理线程 */
    g_alert_running = true;
    
    if (pthread_create(&g_alert_threads[0], NULL, high_priority_alert_thread, NULL) != 0 ||
        pthread_create(&g_alert_threads[1], NULL, normal_priority_alert_thread, NULL) != 0) {
        g_alert_running = false;
        linx_queue_destroy(g_high_priority_queue, free_alert_message);
        linx_queue_destroy(g_normal_priority_queue, free_alert_message);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 关闭告警系统
 */
void alert_shutdown(void)
{
    if (!g_high_priority_queue || !g_normal_priority_queue) return;
    
    /* 停止处理线程 */
    g_alert_running = false;
    
    /* 中断等待的线程 */
    linx_queue_interrupt_all(g_high_priority_queue);
    linx_queue_interrupt_all(g_normal_priority_queue);
    
    /* 等待线程结束 */
    pthread_join(g_alert_threads[0], NULL);
    pthread_join(g_alert_threads[1], NULL);
    
    /* 销毁队列 */
    linx_queue_destroy(g_high_priority_queue, free_alert_message);
    linx_queue_destroy(g_normal_priority_queue, free_alert_message);
    
    g_high_priority_queue = NULL;
    g_normal_priority_queue = NULL;
}

/**
 * @brief 发送告警
 */
int alert_send(alert_type_t type, alert_target_t target, 
               const char *rule_name, const char *message)
{
    alert_message_t *msg;
    linx_queue_t *target_queue;
    linx_queue_result_t result;
    
    if (!g_high_priority_queue || !g_normal_priority_queue) {
        return -1;
    }
    
    /* 创建告警消息 */
    msg = malloc(sizeof(alert_message_t));
    if (!msg) return -1;
    
    msg->type = type;
    msg->target = target;
    msg->rule_name = strdup(rule_name);
    msg->message = strdup(message);
    msg->timestamp = time(NULL);
    msg->priority = (type >= ALERT_TYPE_CRITICAL) ? 1 : 0; // 关键和紧急告警为高优先级
    
    if (!msg->rule_name || !msg->message) {
        free_alert_message(msg);
        return -1;
    }
    
    /* 🔥 根据优先级选择队列 */
    target_queue = (msg->priority == 1) ? g_high_priority_queue : g_normal_priority_queue;
    
    /* 入队 */
    result = linx_queue_push(target_queue, msg);
    if (result != LINX_QUEUE_OK) {
        free_alert_message(msg);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 打印告警统计信息
 */
void alert_print_stats(void)
{
    linx_queue_stats_t high_stats, normal_stats;
    
    if (!g_high_priority_queue || !g_normal_priority_queue) return;
    
    printf("\n=== Alert Queue Statistics ===\n");
    
    if (linx_queue_get_stats(g_high_priority_queue, &high_stats) == LINX_QUEUE_OK) {
        printf("High Priority Queue:\n");
        printf("  Current size: %u\n", high_stats.current_size);
        printf("  Total pushed: %lu\n", high_stats.total_pushed);
        printf("  Total popped: %lu\n", high_stats.total_popped);
        printf("  Total timeouts: %lu\n", high_stats.total_timeouts);
    }
    
    if (linx_queue_get_stats(g_normal_priority_queue, &normal_stats) == LINX_QUEUE_OK) {
        printf("Normal Priority Queue:\n");
        printf("  Current size: %u\n", normal_stats.current_size);
        printf("  Total pushed: %lu\n", normal_stats.total_pushed);
        printf("  Total popped: %lu\n", normal_stats.total_popped);
        printf("  Total timeouts: %lu\n", normal_stats.total_timeouts);
    }
    
    printf("===============================\n\n");
}

/* 示例程序 */
int main(void)
{
    int i;
    
    printf("=== Alert Queue Example ===\n");
    
    /* 初始化告警系统 */
    if (alert_init() != 0) {
        printf("Failed to initialize alert system\n");
        return 1;
    }
    
    /* 发送各种类型的告警 */
    for (i = 0; i < 20; i++) {
        alert_type_t type = i % 4;
        alert_target_t target = i % 4;
        char rule_name[64], message[128];
        
        snprintf(rule_name, sizeof(rule_name), "Rule_%d", i);
        snprintf(message, sizeof(message), "Test alert message %d", i);
        
        alert_send(type, target, rule_name, message);
        
        if (i % 5 == 0) {
            usleep(100000); // 偶尔暂停让队列处理
        }
    }
    
    /* 等待处理完成 */
    sleep(2);
    
    /* 打印统计信息 */
    alert_print_stats();
    
    /* 关闭告警系统 */
    alert_shutdown();
    
    printf("=== Example Complete ===\n");
    return 0;
}