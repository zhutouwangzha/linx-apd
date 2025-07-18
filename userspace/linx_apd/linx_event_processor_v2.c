#include "linx_event_processor_v2.h"
#include "linx_engine.h"
#include "linx_event_rich.h"
#include "linx_rule_engine_set.h"
#include "linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static linx_event_processor_v2_t *g_processor_v2 = NULL;

/* 获取CPU核心数 */
static int get_cpu_count(void)
{
    int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    return (cpu_count > 0) ? cpu_count : 4;
}

/* 获取当前时间戳(毫秒) */
static uint64_t get_current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* 创建事件队列 */
int linx_event_queue_v2_create(linx_event_queue_v2_t **queue, int capacity)
{
    linx_event_queue_v2_t *q;
    
    if (!queue || capacity <= 0) {
        return -1;
    }
    
    q = malloc(sizeof(linx_event_queue_v2_t));
    if (!q) {
        return -1;
    }
    
    q->events = malloc(sizeof(linx_event_data_v2_t) * capacity);
    if (!q->events) {
        free(q);
        return -1;
    }
    
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        free(q->events);
        free(q);
        return -1;
    }
    
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        free(q->events);
        free(q);
        return -1;
    }
    
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        pthread_cond_destroy(&q->not_empty);
        pthread_mutex_destroy(&q->mutex);
        free(q->events);
        free(q);
        return -1;
    }
    
    *queue = q;
    return 0;
}

/* 销毁事件队列 */
void linx_event_queue_v2_destroy(linx_event_queue_v2_t *queue)
{
    if (!queue) {
        return;
    }
    
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);
    
    free(queue->events);
    free(queue);
}

/* 向队列推送事件 */
int linx_event_queue_v2_push(linx_event_queue_v2_t *queue, linx_event_data_v2_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 等待队列非满 */
    while (queue->count >= queue->capacity && !g_processor_v2->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (g_processor_v2->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    /* 添加事件到队列 */
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    /* 通知等待的线程 */
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

/* 从队列弹出事件 */
int linx_event_queue_v2_pop(linx_event_queue_v2_t *queue, linx_event_data_v2_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 等待队列非空 */
    while (queue->count <= 0 && !g_processor_v2->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (g_processor_v2->shutdown && queue->count <= 0) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    /* 从队列取出事件 */
    *event = queue->events[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    /* 通知等待的线程 */
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

/* 批量从队列弹出事件 */
int linx_event_queue_v2_pop_batch(linx_event_queue_v2_t *queue, linx_event_data_v2_t *events, int max_count)
{
    int popped = 0;
    
    if (!queue || !events || max_count <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 等待队列非空 */
    while (queue->count <= 0 && !g_processor_v2->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (g_processor_v2->shutdown && queue->count <= 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    
    /* 批量取出事件 */
    while (queue->count > 0 && popped < max_count) {
        events[popped] = queue->events[queue->head];
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
        popped++;
    }
    
    /* 通知等待的线程 */
    if (popped > 0) {
        pthread_cond_broadcast(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    return popped;
}

/* 事件获取任务 */
void *linx_event_fetch_task(void *arg, int *should_stop)
{
    linx_task_arg_v2_t *task_arg = (linx_task_arg_v2_t *)arg;
    linx_event_data_v2_t event;
    int ret;
    int fetch_count = 1;
    
    if (!task_arg || task_arg->type != TASK_TYPE_EVENT_FETCH) {
        free(arg);
        return NULL;
    }
    
    fetch_count = task_arg->params.fetch_task.fetch_count;
    free(task_arg);
    
    for (int i = 0; i < fetch_count && !*should_stop && !g_processor_v2->shutdown; i++) {
        /* 获取下一个事件 */
        ret = linx_engine_next();
        if (ret != 0) {
            /* 如果没有事件，短暂休眠 */
            usleep(1000); /* 1ms */
            continue;
        }
        
        /* 初始化事件数据 */
        memset(&event, 0, sizeof(event));
        event.timestamp = get_current_time_ms();
        event.event_id = rand();
        event.priority = 0;
        
        /* 事件丰富 */
        ret = linx_event_rich();
        if (ret != 0) {
            LINX_LOG_WARNING("Event enrichment failed");
            pthread_mutex_lock(&g_processor_v2->stats_mutex);
            g_processor_v2->failed_events++;
            pthread_mutex_unlock(&g_processor_v2->stats_mutex);
            continue;
        }
        
        /* 推送到事件队列 */
        ret = linx_event_queue_v2_push(g_processor_v2->event_queue, &event);
        if (ret != 0) {
            LINX_LOG_WARNING("Failed to push event to queue");
            pthread_mutex_lock(&g_processor_v2->stats_mutex);
            g_processor_v2->failed_events++;
            pthread_mutex_unlock(&g_processor_v2->stats_mutex);
            continue;
        }
        
        /* 更新统计信息 */
        pthread_mutex_lock(&g_processor_v2->stats_mutex);
        g_processor_v2->total_events++;
        g_processor_v2->processed_events++;
        pthread_mutex_unlock(&g_processor_v2->stats_mutex);
        
        /* 提交规则匹配任务 */
        linx_event_processor_v2_submit_match_task(&event);
    }
    
    return NULL;
}

/* 规则匹配任务 */
void *linx_rule_match_task(void *arg, int *should_stop)
{
    linx_task_arg_v2_t *task_arg = (linx_task_arg_v2_t *)arg;
    linx_event_data_v2_t event;
    bool matched;
    int ret;
    
    if (!task_arg || task_arg->type != TASK_TYPE_RULE_MATCH) {
        free(arg);
        return NULL;
    }
    
    /* 从队列获取事件 */
    ret = linx_event_queue_v2_pop(g_processor_v2->event_queue, &event);
    if (ret != 0 || *should_stop) {
        free(task_arg);
        return NULL;
    }
    
    /* 执行规则匹配 */
    matched = linx_rule_set_match_rule();
    
    if (matched) {
        /* 更新匹配统计 */
        pthread_mutex_lock(&g_processor_v2->stats_mutex);
        g_processor_v2->matched_events++;
        pthread_mutex_unlock(&g_processor_v2->stats_mutex);
        
        LINX_LOG_DEBUG("Rule matched for event %d", event.event_id);
    }
    
    free(task_arg);
    return NULL;
}

/* 批处理任务 */
void *linx_batch_process_task(void *arg, int *should_stop)
{
    linx_task_arg_v2_t *task_arg = (linx_task_arg_v2_t *)arg;
    linx_event_data_v2_t *events;
    int count;
    bool matched;
    
    if (!task_arg || task_arg->type != TASK_TYPE_BATCH_PROCESS) {
        free(arg);
        return NULL;
    }
    
    events = task_arg->params.batch_task.events;
    count = task_arg->params.batch_task.count;
    
    for (int i = 0; i < count && !*should_stop; i++) {
        /* 执行规则匹配 */
        matched = linx_rule_set_match_rule();
        
        if (matched) {
            /* 更新匹配统计 */
            pthread_mutex_lock(&g_processor_v2->stats_mutex);
            g_processor_v2->matched_events++;
            pthread_mutex_unlock(&g_processor_v2->stats_mutex);
            
            LINX_LOG_DEBUG("Rule matched for event %d", events[i].event_id);
        }
    }
    
    free(events);
    free(task_arg);
    return NULL;
}

/* 提交事件获取任务 */
int linx_event_processor_v2_submit_fetch_tasks(int count)
{
    linx_task_arg_v2_t *task_arg;
    int ret;
    
    if (!g_processor_v2 || !g_processor_v2->running) {
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        task_arg = malloc(sizeof(linx_task_arg_v2_t));
        if (!task_arg) {
            return -1;
        }
        
        task_arg->type = TASK_TYPE_EVENT_FETCH;
        task_arg->params.fetch_task.fetch_count = 1;
        
        ret = linx_thread_pool_add_task(g_processor_v2->fetcher_pool, 
                                       linx_event_fetch_task, task_arg);
        if (ret != 0) {
            free(task_arg);
            return -1;
        }
        
        pthread_mutex_lock(&g_processor_v2->stats_mutex);
        g_processor_v2->fetch_tasks_submitted++;
        pthread_mutex_unlock(&g_processor_v2->stats_mutex);
    }
    
    return 0;
}

/* 提交规则匹配任务 */
int linx_event_processor_v2_submit_match_task(linx_event_data_v2_t *event)
{
    linx_task_arg_v2_t *task_arg;
    int ret;
    
    if (!g_processor_v2 || !g_processor_v2->running || !event) {
        return -1;
    }
    
    task_arg = malloc(sizeof(linx_task_arg_v2_t));
    if (!task_arg) {
        return -1;
    }
    
    task_arg->type = TASK_TYPE_RULE_MATCH;
    task_arg->params.match_task.event = event;
    
    ret = linx_thread_pool_add_task(g_processor_v2->matcher_pool, 
                                   linx_rule_match_task, task_arg);
    if (ret != 0) {
        free(task_arg);
        return -1;
    }
    
    pthread_mutex_lock(&g_processor_v2->stats_mutex);
    g_processor_v2->match_tasks_submitted++;
    pthread_mutex_unlock(&g_processor_v2->stats_mutex);
    
    return 0;
}

/* 初始化事件处理器 */
int linx_event_processor_v2_init(linx_event_processor_v2_config_t *config)
{
    int cpu_count;
    
    if (g_processor_v2) {
        return 0; /* 已经初始化 */
    }
    
    g_processor_v2 = malloc(sizeof(linx_event_processor_v2_t));
    if (!g_processor_v2) {
        return -1;
    }
    
    memset(g_processor_v2, 0, sizeof(linx_event_processor_v2_t));
    
    /* 设置配置 */
    if (config) {
        g_processor_v2->config = *config;
    } else {
        /* 使用默认配置 */
        cpu_count = get_cpu_count();
        g_processor_v2->config.event_fetcher_pool_size = cpu_count;
        g_processor_v2->config.rule_matcher_pool_size = cpu_count * 2;
        g_processor_v2->config.event_queue_size = 1000;
        g_processor_v2->config.batch_size = 10;
    }
    
    /* 创建线程池 */
    g_processor_v2->fetcher_pool = linx_thread_pool_create(g_processor_v2->config.event_fetcher_pool_size);
    if (!g_processor_v2->fetcher_pool) {
        free(g_processor_v2);
        g_processor_v2 = NULL;
        return -1;
    }
    
    g_processor_v2->matcher_pool = linx_thread_pool_create(g_processor_v2->config.rule_matcher_pool_size);
    if (!g_processor_v2->matcher_pool) {
        linx_thread_pool_destroy(g_processor_v2->fetcher_pool, 1);
        free(g_processor_v2);
        g_processor_v2 = NULL;
        return -1;
    }
    
    /* 创建事件队列 */
    if (linx_event_queue_v2_create(&g_processor_v2->event_queue, 
                                  g_processor_v2->config.event_queue_size) != 0) {
        linx_thread_pool_destroy(g_processor_v2->matcher_pool, 1);
        linx_thread_pool_destroy(g_processor_v2->fetcher_pool, 1);
        free(g_processor_v2);
        g_processor_v2 = NULL;
        return -1;
    }
    
    /* 初始化统计信息互斥锁 */
    if (pthread_mutex_init(&g_processor_v2->stats_mutex, NULL) != 0) {
        linx_event_queue_v2_destroy(g_processor_v2->event_queue);
        linx_thread_pool_destroy(g_processor_v2->matcher_pool, 1);
        linx_thread_pool_destroy(g_processor_v2->fetcher_pool, 1);
        free(g_processor_v2);
        g_processor_v2 = NULL;
        return -1;
    }
    
    g_processor_v2->running = false;
    g_processor_v2->shutdown = false;
    g_processor_v2->last_stats_time = get_current_time_ms();
    
    LINX_LOG_INFO("Event processor v2 initialized with %d fetcher threads and %d matcher threads",
                  g_processor_v2->config.event_fetcher_pool_size,
                  g_processor_v2->config.rule_matcher_pool_size);
    
    return 0;
}

/* 启动事件处理器 */
int linx_event_processor_v2_start(void)
{
    if (!g_processor_v2) {
        return -1;
    }
    
    if (g_processor_v2->running) {
        return 0; /* 已经运行 */
    }
    
    g_processor_v2->shutdown = false;
    g_processor_v2->running = true;
    
    /* 提交初始的事件获取任务 */
    int ret = linx_event_processor_v2_submit_fetch_tasks(g_processor_v2->config.event_fetcher_pool_size);
    if (ret != 0) {
        LINX_LOG_ERROR("Failed to submit initial fetch tasks");
        g_processor_v2->running = false;
        return -1;
    }
    
    LINX_LOG_INFO("Event processor v2 started successfully");
    return 0;
}

/* 停止事件处理器 */
int linx_event_processor_v2_stop(void)
{
    if (!g_processor_v2 || !g_processor_v2->running) {
        return 0;
    }
    
    LINX_LOG_INFO("Stopping event processor v2...");
    
    g_processor_v2->shutdown = true;
    g_processor_v2->running = false;
    
    /* 唤醒所有等待的线程 */
    pthread_cond_broadcast(&g_processor_v2->event_queue->not_empty);
    pthread_cond_broadcast(&g_processor_v2->event_queue->not_full);
    
    /* 停止线程池 */
    linx_thread_pool_destroy(g_processor_v2->fetcher_pool, 1);
    linx_thread_pool_destroy(g_processor_v2->matcher_pool, 1);
    
    LINX_LOG_INFO("Event processor v2 stopped");
    return 0;
}

/* 清理事件处理器 */
void linx_event_processor_v2_cleanup(void)
{
    if (!g_processor_v2) {
        return;
    }
    
    if (g_processor_v2->running) {
        linx_event_processor_v2_stop();
    }
    
    pthread_mutex_destroy(&g_processor_v2->stats_mutex);
    linx_event_queue_v2_destroy(g_processor_v2->event_queue);
    
    free(g_processor_v2);
    g_processor_v2 = NULL;
    
    LINX_LOG_INFO("Event processor v2 cleaned up");
}

/* 获取统计信息 */
void linx_event_processor_v2_get_stats(uint64_t *total, uint64_t *processed, uint64_t *matched, uint64_t *failed)
{
    if (!g_processor_v2) {
        if (total) *total = 0;
        if (processed) *processed = 0;
        if (matched) *matched = 0;
        if (failed) *failed = 0;
        return;
    }
    
    pthread_mutex_lock(&g_processor_v2->stats_mutex);
    
    if (total) *total = g_processor_v2->total_events;
    if (processed) *processed = g_processor_v2->processed_events;
    if (matched) *matched = g_processor_v2->matched_events;
    if (failed) *failed = g_processor_v2->failed_events;
    
    pthread_mutex_unlock(&g_processor_v2->stats_mutex);
}

/* 打印统计信息 */
void linx_event_processor_v2_print_stats(void)
{
    uint64_t total, processed, matched, failed;
    int queue_size, fetcher_active, matcher_active;
    
    linx_event_processor_v2_get_stats(&total, &processed, &matched, &failed);
    linx_event_processor_v2_get_queue_status(&queue_size, &fetcher_active, &matcher_active);
    
    LINX_LOG_INFO("=== Event Processor V2 Stats ===");
    LINX_LOG_INFO("Total Events: %lu", total);
    LINX_LOG_INFO("Processed Events: %lu", processed);
    LINX_LOG_INFO("Matched Events: %lu", matched);
    LINX_LOG_INFO("Failed Events: %lu", failed);
    LINX_LOG_INFO("Queue Size: %d", queue_size);
    LINX_LOG_INFO("Active Fetchers: %d", fetcher_active);
    LINX_LOG_INFO("Active Matchers: %d", matcher_active);
    
    if (total > 0) {
        LINX_LOG_INFO("Processing Rate: %.2f%%", (double)processed / total * 100);
    }
    if (processed > 0) {
        LINX_LOG_INFO("Match Rate: %.2f%%", (double)matched / processed * 100);
    }
    LINX_LOG_INFO("===============================");
}

/* 获取队列状态 */
int linx_event_processor_v2_get_queue_status(int *queue_size, int *fetcher_active, int *matcher_active)
{
    if (!g_processor_v2) {
        if (queue_size) *queue_size = 0;
        if (fetcher_active) *fetcher_active = 0;
        if (matcher_active) *matcher_active = 0;
        return -1;
    }
    
    if (queue_size) {
        pthread_mutex_lock(&g_processor_v2->event_queue->mutex);
        *queue_size = g_processor_v2->event_queue->count;
        pthread_mutex_unlock(&g_processor_v2->event_queue->mutex);
    }
    
    if (fetcher_active) {
        *fetcher_active = linx_thread_pool_get_active_threads(g_processor_v2->fetcher_pool);
    }
    
    if (matcher_active) {
        *matcher_active = linx_thread_pool_get_active_threads(g_processor_v2->matcher_pool);
    }
    
    return 0;
}