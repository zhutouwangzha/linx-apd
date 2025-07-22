#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include "linx_event_processor.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_rule_engine_set.h"
#include "linx_alert.h"
#include "event.h"

static linx_event_processor_t *g_event_processor = NULL;

/* 获取CPU核数 */
static int get_cpu_count(void)
{
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

/* 获取当前时间（毫秒） */
static uint64_t get_current_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/* 事件获取任务参数 */
typedef struct {
    linx_ebpf_t *ebpf_manager;
    int worker_id;
} event_fetch_task_arg_t;

/* 事件匹配任务参数 */
typedef struct {
    event_t *event;
    int worker_id;
} event_match_task_arg_t;

/* 前向声明匹配工作函数 */
static void *event_match_worker(void *arg, int *should_stop);

/* 事件获取工作函数 */
static void *event_fetch_worker(void *arg, int *should_stop)
{
    event_fetch_task_arg_t *task_arg = (event_fetch_task_arg_t *)arg;
    linx_event_t *raw_event = NULL;
    event_t *processed_event = NULL;
    int ret;

    if (!task_arg || !task_arg->ebpf_manager) {
        LINX_LOG_ERROR("Invalid task arguments for event fetch worker %d", 
                       task_arg ? task_arg->worker_id : -1);
        return NULL;
    }

    LINX_LOG_DEBUG("Event fetch worker %d started", task_arg->worker_id);

    while (!*should_stop && !g_event_processor->shutdown) {
        /* 从eBPF环形缓冲区获取原始事件 */
        ret = linx_ebpf_get_ringbuf_msg(task_arg->ebpf_manager, &raw_event);
        if (ret != 0 || !raw_event) {
            /* 没有事件时短暂休眠，避免CPU占用过高 */
            usleep(1000);  /* 1ms */
            continue;
        }

        /* 更新统计信息 */
        pthread_mutex_lock(&g_event_processor->status_mutex);
        g_event_processor->total_events++;
        pthread_mutex_unlock(&g_event_processor->status_mutex);

        /* 事件丰富化处理 - 转换为event_t结构 */
        processed_event = malloc(sizeof(event_t));
        if (!processed_event) {
            LINX_LOG_ERROR("Failed to allocate memory for processed event");
            pthread_mutex_lock(&g_event_processor->status_mutex);
            g_event_processor->failed_events++;
            pthread_mutex_unlock(&g_event_processor->status_mutex);
            continue;
        }

        /* TODO: 这里需要实现从linx_event_t到event_t的转换逻辑 */
        /* 暂时填充基本字段作为示例 */
        memset(processed_event, 0, sizeof(event_t));
        processed_event->num = g_event_processor->total_events;
        snprintf(processed_event->time, sizeof(processed_event->time), "%lu", get_current_time_ms());
        processed_event->type = "unknown";  /* 需要根据raw_event填充 */
        processed_event->rawres = 0;
        processed_event->failed = false;
        processed_event->dir[0] = '>';

        /* 创建匹配任务并提交到匹配线程池 */
        event_match_task_arg_t *match_arg = malloc(sizeof(event_match_task_arg_t));
        if (!match_arg) {
            LINX_LOG_ERROR("Failed to allocate memory for match task");
            free(processed_event);
            pthread_mutex_lock(&g_event_processor->status_mutex);
            g_event_processor->failed_events++;
            pthread_mutex_unlock(&g_event_processor->status_mutex);
            continue;
        }

        match_arg->event = processed_event;
        match_arg->worker_id = task_arg->worker_id;

        ret = linx_thread_pool_add_task(g_event_processor->matcher_pool, 
                                       event_match_worker, match_arg);
        if (ret != 0) {
            LINX_LOG_ERROR("Failed to submit match task to thread pool");
            free(match_arg);
            free(processed_event);
            pthread_mutex_lock(&g_event_processor->status_mutex);
            g_event_processor->failed_events++;
            pthread_mutex_unlock(&g_event_processor->status_mutex);
            continue;
        }

        pthread_mutex_lock(&g_event_processor->status_mutex);
        g_event_processor->match_tasks_submitted++;
        pthread_mutex_unlock(&g_event_processor->status_mutex);
    }

    LINX_LOG_DEBUG("Event fetch worker %d stopped", task_arg->worker_id);
    return NULL;
}

/* 事件匹配工作函数 */
static void *event_match_worker(void *arg, int *should_stop)
{
    event_match_task_arg_t *task_arg = (event_match_task_arg_t *)arg;
    bool match_result = false;
    linx_rule_set_t *rule_set = NULL;
    int i;

    if (!task_arg || !task_arg->event) {
        LINX_LOG_ERROR("Invalid task arguments for event match worker");
        if (task_arg) {
            free(task_arg);
        }
        return NULL;
    }

    LINX_LOG_DEBUG("Event match worker processing event %lu", task_arg->event->num);

    /* TODO: 设置当前事件到全局上下文，供规则匹配使用 */
    /* 这里需要实现一个线程安全的方式来传递当前事件给规则引擎 */

    /* 获取规则集合并进行匹配 */
    rule_set = linx_rule_set_get();
    if (rule_set && rule_set->size > 0) {
        /* 遍历所有规则进行匹配 */
        for (i = 0; i < rule_set->size; i++) {
            if (rule_set->data.matches[i]) {
                match_result = linx_rule_engine_match(rule_set->data.matches[i]);
                if (match_result) {
                    /* 规则匹配成功，触发输出 */
                    LINX_LOG_INFO("Rule matched for event %lu", task_arg->event->num);
                    
                    if (rule_set->data.outputs[i]) {
                        /* 异步发送告警 */
                        char rule_name[256];
                        snprintf(rule_name, sizeof(rule_name), "rule_%d", i);
                        
                        int ret = linx_alert_send_async(rule_set->data.outputs[i], 
                                                       rule_name, 1);
                        if (ret != 0) {
                            LINX_LOG_ERROR("Failed to send alert for matched rule");
                        }
                    }

                    pthread_mutex_lock(&g_event_processor->status_mutex);
                    g_event_processor->matched_events++;
                    pthread_mutex_unlock(&g_event_processor->status_mutex);
                    
                    /* 如果配置为匹配第一个规则就停止，可以在这里break */
                }
            }
        }
    }

    /* 更新处理统计 */
    pthread_mutex_lock(&g_event_processor->status_mutex);
    g_event_processor->processed_events++;
    pthread_mutex_unlock(&g_event_processor->status_mutex);

    /* 释放资源 */
    free(task_arg->event);
    free(task_arg);

    return NULL;
}

int linx_event_processor_init(linx_event_processor_config_t *config)
{
    int cpu_count;

    if (g_event_processor) {
        LINX_LOG_WARN("Event processor already initialized");
        return 0;
    }

    g_event_processor = malloc(sizeof(linx_event_processor_t));
    if (!g_event_processor) {
        LINX_LOG_ERROR("Failed to allocate memory for event processor");
        return -1;
    }

    memset(g_event_processor, 0, sizeof(linx_event_processor_t));

    /* 设置配置参数 */
    if (config) {
        g_event_processor->config = *config;
    } else {
        cpu_count = get_cpu_count();
        g_event_processor->config.event_fetcher_pool_size = cpu_count;
        g_event_processor->config.event_matcher_pool_size = cpu_count * 2;
        g_event_processor->config.event_queue_size = 1000;
        g_event_processor->config.batch_size = 10;
    }

    /* 创建事件获取线程池 */
    g_event_processor->fetcher_pool = linx_thread_pool_create(
        g_event_processor->config.event_fetcher_pool_size);
    if (!g_event_processor->fetcher_pool) {
        LINX_LOG_ERROR("Failed to create event fetcher thread pool");
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    /* 创建事件匹配线程池 */
    g_event_processor->matcher_pool = linx_thread_pool_create(
        g_event_processor->config.event_matcher_pool_size);
    if (!g_event_processor->matcher_pool) {
        LINX_LOG_ERROR("Failed to create event matcher thread pool");
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 1);
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    /* 初始化互斥锁 */
    if (pthread_mutex_init(&g_event_processor->status_mutex, NULL) != 0) {
        LINX_LOG_ERROR("Failed to initialize status mutex");
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 1);
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    /* 初始化状态 */
    g_event_processor->running = false;
    g_event_processor->shutdown = false;
    g_event_processor->last_stats_time = get_current_time_ms();

    LINX_LOG_INFO("Event processor initialized with %d fetcher threads and %d matcher threads",
                  g_event_processor->config.event_fetcher_pool_size,
                  g_event_processor->config.event_matcher_pool_size);

    return 0;
}

int linx_event_processor_start(linx_ebpf_t *ebpf_manager)
{
    int i, ret;
    event_fetch_task_arg_t *task_arg;

    if (!g_event_processor) {
        LINX_LOG_ERROR("Event processor not initialized");
        return -1;
    }

    if (!ebpf_manager) {
        LINX_LOG_ERROR("Invalid eBPF manager");
        return -1;
    }

    if (g_event_processor->running) {
        LINX_LOG_WARN("Event processor already running");
        return 0;
    }

    /* 设置运行状态 */
    pthread_mutex_lock(&g_event_processor->status_mutex);
    g_event_processor->running = true;
    g_event_processor->shutdown = false;
    pthread_mutex_unlock(&g_event_processor->status_mutex);

    /* 为每个获取线程创建并提交持续运行的任务 */
    for (i = 0; i < g_event_processor->config.event_fetcher_pool_size; i++) {
        task_arg = malloc(sizeof(event_fetch_task_arg_t));
        if (!task_arg) {
            LINX_LOG_ERROR("Failed to allocate memory for fetch task %d", i);
            continue;
        }

        task_arg->ebpf_manager = ebpf_manager;
        task_arg->worker_id = i;

        ret = linx_thread_pool_add_task(g_event_processor->fetcher_pool,
                                       event_fetch_worker, task_arg);
        if (ret != 0) {
            LINX_LOG_ERROR("Failed to submit fetch task %d to thread pool", i);
            free(task_arg);
            continue;
        }

        pthread_mutex_lock(&g_event_processor->status_mutex);
        g_event_processor->fetch_tasks_submitted++;
        pthread_mutex_unlock(&g_event_processor->status_mutex);
    }

    LINX_LOG_INFO("Event processor started with %lu fetch tasks",
                  g_event_processor->fetch_tasks_submitted);

    return 0;
}

int linx_event_processor_stop(void)
{
    if (!g_event_processor) {
        LINX_LOG_ERROR("Event processor not initialized");
        return -1;
    }

    if (!g_event_processor->running) {
        LINX_LOG_WARN("Event processor not running");
        return 0;
    }

    LINX_LOG_INFO("Stopping event processor...");

    /* 设置停止标志 */
    pthread_mutex_lock(&g_event_processor->status_mutex);
    g_event_processor->shutdown = true;
    g_event_processor->running = false;
    pthread_mutex_unlock(&g_event_processor->status_mutex);

    /* 等待线程池停止 */
    if (g_event_processor->fetcher_pool) {
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 1);  /* 优雅停止 */
        g_event_processor->fetcher_pool = NULL;
    }

    if (g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);  /* 优雅停止 */
        g_event_processor->matcher_pool = NULL;
    }

    LINX_LOG_INFO("Event processor stopped");
    return 0;
}

void linx_event_processor_deinit(void)
{
    if (!g_event_processor) {
        return;
    }

    /* 确保先停止处理器 */
    if (g_event_processor->running) {
        linx_event_processor_stop();
    }

    /* 销毁互斥锁 */
    pthread_mutex_destroy(&g_event_processor->status_mutex);

    /* 释放内存 */
    free(g_event_processor);
    g_event_processor = NULL;

    LINX_LOG_INFO("Event processor deinitialized");
}

void linx_event_processor_get_stats(linx_event_processor_stats_t *stats)
{
    if (!g_event_processor || !stats) {
        return;
    }

    pthread_mutex_lock(&g_event_processor->status_mutex);
    
    stats->total_events = g_event_processor->total_events;
    stats->processed_events = g_event_processor->processed_events;
    stats->matched_events = g_event_processor->matched_events;
    stats->failed_events = g_event_processor->failed_events;
    stats->fetch_tasks_submitted = g_event_processor->fetch_tasks_submitted;
    stats->match_tasks_submitted = g_event_processor->match_tasks_submitted;
    
    stats->fetcher_active_threads = g_event_processor->fetcher_pool ? 
        linx_thread_pool_get_active_threads(g_event_processor->fetcher_pool) : 0;
    stats->matcher_active_threads = g_event_processor->matcher_pool ? 
        linx_thread_pool_get_active_threads(g_event_processor->matcher_pool) : 0;
    
    stats->fetcher_queue_size = g_event_processor->fetcher_pool ? 
        linx_thread_pool_get_queue_size(g_event_processor->fetcher_pool) : 0;
    stats->matcher_queue_size = g_event_processor->matcher_pool ? 
        linx_thread_pool_get_queue_size(g_event_processor->matcher_pool) : 0;

    stats->running = g_event_processor->running;

    pthread_mutex_unlock(&g_event_processor->status_mutex);
}

bool linx_event_processor_is_running(void)
{
    if (!g_event_processor) {
        return false;
    }

    bool running;
    pthread_mutex_lock(&g_event_processor->status_mutex);
    running = g_event_processor->running;
    pthread_mutex_unlock(&g_event_processor->status_mutex);

    return running;
}
