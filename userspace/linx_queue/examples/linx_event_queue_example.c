/**
 * @file linx_event_queue_example.c
 * @brief 演示如何在事件处理模块中使用通用队列
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "linx_queue.h"

/* 模拟事件结构体 */
typedef struct {
    uint32_t event_id;
    uint32_t event_type;
    uint64_t timestamp;
    uint32_t pid;
    char *process_name;
    char *event_data;
    size_t data_size;
} linx_event_t;

/* 事件处理器结构体 */
typedef struct {
    int worker_id;
    pthread_t thread;
    linx_queue_t *input_queue;
    volatile bool running;
} event_processor_t;

/* 全局变量 */
static linx_queue_t *g_event_queue = NULL;           // 原始事件队列
static linx_queue_t *g_enriched_queue = NULL;        // 丰富后事件队列
static event_processor_t g_processors[4];            // 4个处理器
static pthread_t g_fetcher_thread;                   // 事件获取线程
static pthread_t g_matcher_thread;                   // 规则匹配线程
static volatile bool g_system_running = false;

/**
 * @brief 释放事件结构体
 */
static void free_event(void *data)
{
    linx_event_t *event = (linx_event_t *)data;
    if (event) {
        free(event->process_name);
        free(event->event_data);
        free(event);
    }
}

/**
 * @brief 创建模拟事件
 */
static linx_event_t *create_mock_event(uint32_t id)
{
    linx_event_t *event = malloc(sizeof(linx_event_t));
    if (!event) return NULL;
    
    event->event_id = id;
    event->event_type = id % 10;  // 10种事件类型
    event->timestamp = time(NULL) * 1000000 + (id % 1000000);
    event->pid = 1000 + (id % 9000);
    
    char process_name[64], event_data[256];
    snprintf(process_name, sizeof(process_name), "process_%u", event->pid);
    snprintf(event_data, sizeof(event_data), 
             "Event data for ID %u, type %u, timestamp %lu", 
             id, event->event_type, event->timestamp);
    
    event->process_name = strdup(process_name);
    event->event_data = strdup(event_data);
    event->data_size = strlen(event_data);
    
    if (!event->process_name || !event->event_data) {
        free_event(event);
        return NULL;
    }
    
    return event;
}

/**
 * @brief 模拟事件丰富处理
 */
static void enrich_event(linx_event_t *event)
{
    /* 模拟事件丰富：添加更多上下文信息 */
    char *old_data = event->event_data;
    char enriched_data[512];
    
    snprintf(enriched_data, sizeof(enriched_data),
             "%s | Enriched: parent_pid=%u, user=user_%u, cwd=/tmp/work_%u",
             old_data, event->pid - 1, event->pid % 1000, event->pid);
    
    event->event_data = strdup(enriched_data);
    event->data_size = strlen(enriched_data);
    
    free(old_data);
    
    /* 模拟丰富处理时间 */
    usleep(1000); // 1ms
}

/**
 * @brief 模拟规则匹配
 */
static bool match_rules(linx_event_t *event)
{
    /* 模拟规则匹配：某些事件类型触发告警 */
    bool matched = (event->event_type == 3 || event->event_type == 7 || event->event_type == 9);
    
    if (matched) {
        printf("[RULE_MATCH] Event %u (type %u) matched security rule\n", 
               event->event_id, event->event_type);
    }
    
    /* 模拟规则匹配时间 */
    usleep(500); // 0.5ms
    
    return matched;
}

/**
 * @brief 事件处理工作线程
 */
static void *event_processor_thread(void *arg)
{
    event_processor_t *processor = (event_processor_t *)arg;
    linx_event_t *event;
    linx_queue_result_t result;
    
    printf("[PROCESSOR_%d] Started\n", processor->worker_id);
    
    while (processor->running) {
        /* 🔥 从输入队列获取事件，100ms超时 */
        result = linx_queue_pop_timeout(processor->input_queue, (void **)&event, 100);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            break;
        } else if (result != LINX_QUEUE_OK) {
            continue;
        }
        
        /* 丰富事件 */
        enrich_event(event);
        
        /* 🔥 将丰富后的事件放入下一级队列 */
        if (linx_queue_push(g_enriched_queue, event) != LINX_QUEUE_OK) {
            printf("[PROCESSOR_%d] Failed to push enriched event\n", processor->worker_id);
            free_event(event);
        }
    }
    
    printf("[PROCESSOR_%d] Stopped\n", processor->worker_id);
    return NULL;
}

/**
 * @brief 事件获取线程（模拟从内核获取事件）
 */
static void *event_fetcher_thread(void *arg)
{
    (void)arg;
    uint32_t event_id = 0;
    linx_event_t *event;
    
    printf("[FETCHER] Started\n");
    
    while (g_system_running) {
        /* 模拟从内核获取事件 */
        event = create_mock_event(event_id++);
        if (!event) {
            usleep(1000);
            continue;
        }
        
        /* 🔥 将原始事件放入处理队列 */
        if (linx_queue_push(g_event_queue, event) != LINX_QUEUE_OK) {
            printf("[FETCHER] Event queue full, dropping event %u\n", event->event_id);
            free_event(event);
        }
        
        /* 模拟事件到达频率 */
        usleep(5000); // 5ms间隔，每秒200个事件
    }
    
    printf("[FETCHER] Stopped\n");
    return NULL;
}

/**
 * @brief 规则匹配线程
 */
static void *rule_matcher_thread(void *arg)
{
    (void)arg;
    linx_event_t *event;
    linx_queue_result_t result;
    
    printf("[MATCHER] Started\n");
    
    while (g_system_running) {
        /* 🔥 从丰富队列获取事件进行规则匹配 */
        result = linx_queue_pop_timeout(g_enriched_queue, (void **)&event, 100);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            break;
        } else if (result != LINX_QUEUE_OK) {
            continue;
        }
        
        /* 执行规则匹配 */
        bool matched = match_rules(event);
        
        if (matched) {
            /* 这里可以发送告警 */
            printf("[MATCHER] Alert triggered for event %u\n", event->event_id);
        }
        
        /* 处理完成，释放事件 */
        free_event(event);
    }
    
    printf("[MATCHER] Stopped\n");
    return NULL;
}

/**
 * @brief 初始化事件处理系统
 */
int event_system_init(void)
{
    linx_queue_config_t raw_config = {
        .initial_capacity = 1024,     // 原始事件队列较大
        .max_capacity = 10240,        // 限制最大容量防止内存爆炸
        .auto_resize = true,
        .resize_factor = 2,
        .thread_safe = true,
        .use_condition = true
    };
    
    linx_queue_config_t enriched_config = {
        .initial_capacity = 512,      // 丰富队列较小
        .max_capacity = 5120,
        .auto_resize = true,
        .resize_factor = 2,
        .thread_safe = true,
        .use_condition = true
    };
    
    /* 创建队列 */
    g_event_queue = linx_queue_create(&raw_config);
    g_enriched_queue = linx_queue_create(&enriched_config);
    
    if (!g_event_queue || !g_enriched_queue) {
        if (g_event_queue) linx_queue_destroy(g_event_queue, free_event);
        if (g_enriched_queue) linx_queue_destroy(g_enriched_queue, free_event);
        return -1;
    }
    
    /* 初始化事件处理器 */
    for (int i = 0; i < 4; i++) {
        g_processors[i].worker_id = i;
        g_processors[i].input_queue = g_event_queue;  // 所有处理器共享输入队列
        g_processors[i].running = true;
    }
    
    /* 启动所有线程 */
    g_system_running = true;
    
    /* 启动事件处理器线程 */
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&g_processors[i].thread, NULL, 
                          event_processor_thread, &g_processors[i]) != 0) {
            printf("Failed to create processor thread %d\n", i);
            return -1;
        }
    }
    
    /* 启动获取线程 */
    if (pthread_create(&g_fetcher_thread, NULL, event_fetcher_thread, NULL) != 0) {
        printf("Failed to create fetcher thread\n");
        return -1;
    }
    
    /* 启动匹配线程 */
    if (pthread_create(&g_matcher_thread, NULL, rule_matcher_thread, NULL) != 0) {
        printf("Failed to create matcher thread\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief 关闭事件处理系统
 */
void event_system_shutdown(void)
{
    /* 停止所有线程 */
    g_system_running = false;
    
    for (int i = 0; i < 4; i++) {
        g_processors[i].running = false;
    }
    
    /* 中断等待的线程 */
    if (g_event_queue) linx_queue_interrupt_all(g_event_queue);
    if (g_enriched_queue) linx_queue_interrupt_all(g_enriched_queue);
    
    /* 等待线程结束 */
    pthread_join(g_fetcher_thread, NULL);
    pthread_join(g_matcher_thread, NULL);
    
    for (int i = 0; i < 4; i++) {
        pthread_join(g_processors[i].thread, NULL);
    }
    
    /* 销毁队列 */
    if (g_event_queue) {
        linx_queue_destroy(g_event_queue, free_event);
        g_event_queue = NULL;
    }
    
    if (g_enriched_queue) {
        linx_queue_destroy(g_enriched_queue, free_event);
        g_enriched_queue = NULL;
    }
}

/**
 * @brief 打印系统统计信息
 */
void event_system_print_stats(void)
{
    linx_queue_stats_t raw_stats, enriched_stats;
    
    printf("\n=== Event System Statistics ===\n");
    
    if (g_event_queue && linx_queue_get_stats(g_event_queue, &raw_stats) == LINX_QUEUE_OK) {
        printf("Raw Event Queue:\n");
        printf("  Current size: %u\n", raw_stats.current_size);
        printf("  Current capacity: %u\n", raw_stats.current_capacity);
        printf("  Total pushed: %lu\n", raw_stats.total_pushed);
        printf("  Total popped: %lu\n", raw_stats.total_popped);
        printf("  Total timeouts: %lu\n", raw_stats.total_timeouts);
        printf("  Total resizes: %lu\n", raw_stats.total_resizes);
    }
    
    if (g_enriched_queue && linx_queue_get_stats(g_enriched_queue, &enriched_stats) == LINX_QUEUE_OK) {
        printf("Enriched Event Queue:\n");
        printf("  Current size: %u\n", enriched_stats.current_size);
        printf("  Current capacity: %u\n", enriched_stats.current_capacity);
        printf("  Total pushed: %lu\n", enriched_stats.total_pushed);
        printf("  Total popped: %lu\n", enriched_stats.total_popped);
        printf("  Total timeouts: %lu\n", enriched_stats.total_timeouts);
        printf("  Total resizes: %lu\n", enriched_stats.total_resizes);
    }
    
    printf("================================\n\n");
}

/* 示例程序 */
int main(void)
{
    printf("=== Event Processing System Example ===\n");
    
    /* 初始化事件系统 */
    if (event_system_init() != 0) {
        printf("Failed to initialize event system\n");
        return 1;
    }
    
    printf("Event system started, processing events for 10 seconds...\n");
    
    /* 运行10秒 */
    sleep(10);
    
    /* 打印统计信息 */
    event_system_print_stats();
    
    /* 关闭系统 */
    printf("Shutting down event system...\n");
    event_system_shutdown();
    
    printf("=== Example Complete ===\n");
    return 0;
}