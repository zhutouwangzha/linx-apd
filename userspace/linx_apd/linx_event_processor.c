#include "linx_event_processor.h"
#include "linx_engine.h"
#include "linx_event_rich.h"
#include "linx_rule_engine_set.h"
#include "linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static linx_event_processor_t *g_processor = NULL;

/* 获取CPU核心数 */
static int get_cpu_count(void)
{
    int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    return (cpu_count > 0) ? cpu_count : 4;
}

/* 创建事件队列 */
int linx_event_queue_create(linx_event_queue_t **queue, int capacity)
{
    linx_event_queue_t *q;
    
    if (!queue || capacity <= 0) {
        return -1;
    }
    
    q = malloc(sizeof(linx_event_queue_t));
    if (!q) {
        return -1;
    }
    
    q->events = malloc(sizeof(linx_event_data_t) * capacity);
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
void linx_event_queue_destroy(linx_event_queue_t *queue)
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
int linx_event_queue_push_event(linx_event_queue_t *queue, linx_event_data_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 等待队列非满 */
    while (queue->count >= queue->capacity && !g_processor->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (g_processor->shutdown) {
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
int linx_event_queue_pop_event(linx_event_queue_t *queue, linx_event_data_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 等待队列非空 */
    while (queue->count <= 0 && !g_processor->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (g_processor->shutdown && queue->count <= 0) {
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

/* 事件获取线程 */
void *linx_event_fetcher_thread(void *arg)
{
    (void)arg;
    linx_event_data_t event;
    int ret;
    
    LINX_LOG_INFO("Event fetcher thread started");
    
    while (!g_processor->shutdown) {
        /* 获取下一个事件 */
        ret = linx_engine_next();
        if (ret != 0) {
            /* 如果没有事件，短暂休眠 */
            usleep(1000); /* 1ms */
            continue;
        }
        
        /* 初始化事件数据 */
        memset(&event, 0, sizeof(event));
        event.timestamp = time(NULL);
        event.event_id = rand(); /* 临时使用随机ID */
        
        /* 事件丰富 */
        ret = linx_event_rich();
        if (ret != 0) {
            LINX_LOG_WARNING("Event enrichment failed");
            continue;
        }
        
        /* 推送到丰富事件队列 */
        ret = linx_event_queue_push_event(g_processor->enriched_queue, &event);
        if (ret != 0) {
            LINX_LOG_WARNING("Failed to push enriched event to queue");
            continue;
        }
        
        /* 更新统计信息 */
        pthread_mutex_lock(&g_processor->stats_mutex);
        g_processor->total_events++;
        g_processor->processed_events++;
        pthread_mutex_unlock(&g_processor->stats_mutex);
    }
    
    LINX_LOG_INFO("Event fetcher thread stopped");
    return NULL;
}

/* 规则匹配线程 */
void *linx_rule_matcher_thread(void *arg)
{
    (void)arg;
    linx_event_data_t event;
    int ret;
    bool matched;
    
    LINX_LOG_INFO("Rule matcher thread started");
    
    while (!g_processor->shutdown) {
        /* 从丰富事件队列获取事件 */
        ret = linx_event_queue_pop_event(g_processor->enriched_queue, &event);
        if (ret != 0) {
            continue;
        }
        
        /* 执行规则匹配 */
        matched = linx_rule_set_match_rule();
        
        if (matched) {
            /* 更新匹配统计 */
            pthread_mutex_lock(&g_processor->stats_mutex);
            g_processor->matched_events++;
            pthread_mutex_unlock(&g_processor->stats_mutex);
            
            LINX_LOG_DEBUG("Rule matched for event %d", event.event_id);
        }
    }
    
    LINX_LOG_INFO("Rule matcher thread stopped");
    return NULL;
}

/* 初始化事件处理器 */
int linx_event_processor_init(linx_event_processor_config_t *config)
{
    int cpu_count;
    
    if (g_processor) {
        return 0; /* 已经初始化 */
    }
    
    g_processor = malloc(sizeof(linx_event_processor_t));
    if (!g_processor) {
        return -1;
    }
    
    memset(g_processor, 0, sizeof(linx_event_processor_t));
    
    /* 设置配置 */
    if (config) {
        g_processor->config = *config;
    } else {
        /* 使用默认配置 */
        cpu_count = get_cpu_count();
        g_processor->config.event_fetcher_threads = cpu_count;
        g_processor->config.rule_matcher_threads = cpu_count * 2;
        g_processor->config.event_queue_size = 1000;
        g_processor->config.enriched_queue_size = 2000;
    }
    
    /* 创建事件队列 */
    if (linx_event_queue_create(&g_processor->raw_queue, 
                               g_processor->config.event_queue_size) != 0) {
        free(g_processor);
        g_processor = NULL;
        return -1;
    }
    
    if (linx_event_queue_create(&g_processor->enriched_queue, 
                               g_processor->config.enriched_queue_size) != 0) {
        linx_event_queue_destroy(g_processor->raw_queue);
        free(g_processor);
        g_processor = NULL;
        return -1;
    }
    
    /* 分配线程数组 */
    g_processor->fetcher_threads = malloc(sizeof(pthread_t) * 
                                         g_processor->config.event_fetcher_threads);
    if (!g_processor->fetcher_threads) {
        linx_event_queue_destroy(g_processor->enriched_queue);
        linx_event_queue_destroy(g_processor->raw_queue);
        free(g_processor);
        g_processor = NULL;
        return -1;
    }
    
    g_processor->matcher_threads = malloc(sizeof(pthread_t) * 
                                         g_processor->config.rule_matcher_threads);
    if (!g_processor->matcher_threads) {
        free(g_processor->fetcher_threads);
        linx_event_queue_destroy(g_processor->enriched_queue);
        linx_event_queue_destroy(g_processor->raw_queue);
        free(g_processor);
        g_processor = NULL;
        return -1;
    }
    
    /* 初始化统计信息互斥锁 */
    if (pthread_mutex_init(&g_processor->stats_mutex, NULL) != 0) {
        free(g_processor->matcher_threads);
        free(g_processor->fetcher_threads);
        linx_event_queue_destroy(g_processor->enriched_queue);
        linx_event_queue_destroy(g_processor->raw_queue);
        free(g_processor);
        g_processor = NULL;
        return -1;
    }
    
    g_processor->running = false;
    g_processor->shutdown = false;
    
    LINX_LOG_INFO("Event processor initialized with %d fetcher threads and %d matcher threads",
                  g_processor->config.event_fetcher_threads,
                  g_processor->config.rule_matcher_threads);
    
    return 0;
}

/* 启动事件处理器 */
int linx_event_processor_start(void)
{
    int i;
    
    if (!g_processor) {
        return -1;
    }
    
    if (g_processor->running) {
        return 0; /* 已经运行 */
    }
    
    g_processor->shutdown = false;
    g_processor->running = true;
    
    /* 创建事件获取线程 */
    for (i = 0; i < g_processor->config.event_fetcher_threads; i++) {
        if (pthread_create(&g_processor->fetcher_threads[i], NULL,
                          linx_event_fetcher_thread, NULL) != 0) {
            LINX_LOG_ERROR("Failed to create event fetcher thread %d", i);
            linx_event_processor_stop();
            return -1;
        }
    }
    
    /* 创建规则匹配线程 */
    for (i = 0; i < g_processor->config.rule_matcher_threads; i++) {
        if (pthread_create(&g_processor->matcher_threads[i], NULL,
                          linx_rule_matcher_thread, NULL) != 0) {
            LINX_LOG_ERROR("Failed to create rule matcher thread %d", i);
            linx_event_processor_stop();
            return -1;
        }
    }
    
    LINX_LOG_INFO("Event processor started successfully");
    return 0;
}

/* 停止事件处理器 */
int linx_event_processor_stop(void)
{
    int i;
    
    if (!g_processor || !g_processor->running) {
        return 0;
    }
    
    LINX_LOG_INFO("Stopping event processor...");
    
    g_processor->shutdown = true;
    
    /* 唤醒所有等待的线程 */
    pthread_cond_broadcast(&g_processor->raw_queue->not_empty);
    pthread_cond_broadcast(&g_processor->raw_queue->not_full);
    pthread_cond_broadcast(&g_processor->enriched_queue->not_empty);
    pthread_cond_broadcast(&g_processor->enriched_queue->not_full);
    
    /* 等待事件获取线程结束 */
    for (i = 0; i < g_processor->config.event_fetcher_threads; i++) {
        pthread_join(g_processor->fetcher_threads[i], NULL);
    }
    
    /* 等待规则匹配线程结束 */
    for (i = 0; i < g_processor->config.rule_matcher_threads; i++) {
        pthread_join(g_processor->matcher_threads[i], NULL);
    }
    
    g_processor->running = false;
    
    LINX_LOG_INFO("Event processor stopped");
    return 0;
}

/* 清理事件处理器 */
void linx_event_processor_cleanup(void)
{
    if (!g_processor) {
        return;
    }
    
    if (g_processor->running) {
        linx_event_processor_stop();
    }
    
    pthread_mutex_destroy(&g_processor->stats_mutex);
    
    free(g_processor->matcher_threads);
    free(g_processor->fetcher_threads);
    
    linx_event_queue_destroy(g_processor->enriched_queue);
    linx_event_queue_destroy(g_processor->raw_queue);
    
    free(g_processor);
    g_processor = NULL;
    
    LINX_LOG_INFO("Event processor cleaned up");
}

/* 获取统计信息 */
void linx_event_processor_get_stats(uint64_t *total, uint64_t *processed, uint64_t *matched)
{
    if (!g_processor) {
        if (total) *total = 0;
        if (processed) *processed = 0;
        if (matched) *matched = 0;
        return;
    }
    
    pthread_mutex_lock(&g_processor->stats_mutex);
    
    if (total) *total = g_processor->total_events;
    if (processed) *processed = g_processor->processed_events;
    if (matched) *matched = g_processor->matched_events;
    
    pthread_mutex_unlock(&g_processor->stats_mutex);
}