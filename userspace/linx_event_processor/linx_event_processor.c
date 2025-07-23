#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <stdatomic.h>
#include <sys/queue.h>

#include "linx_event_processor.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_rule_engine_set.h"
#include "linx_alert.h"
#include "event.h"

/* 内部宏定义 */
#define LINX_NSEC_PER_SEC           1000000000UL
#define LINX_USEC_PER_SEC           1000000UL
#define LINX_MSEC_PER_SEC           1000UL
#define LINX_SPIN_LOCK_ATTEMPTS     1000
#define LINX_MEMORY_ALIGNMENT       64
#define LINX_CACHE_LINE_SIZE        64

/* 任务类型定义 */
typedef enum {
    LINX_TASK_TYPE_FETCH_EVENT,
    LINX_TASK_TYPE_MATCH_EVENT,
    LINX_TASK_TYPE_SHUTDOWN
} linx_task_type_t;

/* 任务参数结构 */
typedef struct {
    linx_task_type_t type;
    linx_event_processor_t *processor;
    linx_event_wrapper_t *event_wrapper;
    int worker_id;
} linx_task_arg_t;

/* 内部工具函数 */
static inline uint64_t get_timestamp_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * LINX_NSEC_PER_SEC + (uint64_t)ts.tv_nsec;
}

static inline uint64_t get_timestamp_us(void)
{
    return get_timestamp_ns() / 1000;
}

static inline uint32_t get_cpu_count(void)
{
    return (uint32_t)get_nprocs();
}

static inline void set_thread_name(const char *name)
{
    pthread_setname_np(pthread_self(), name);
}

static inline int set_cpu_affinity(int cpu_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

/* 设置错误信息 */
static void set_last_error(linx_event_processor_t *processor, const char *error)
{
    if (!processor || !error) return;
    
    pthread_mutex_lock(&processor->error_mutex);
    strncpy(processor->last_error, error, sizeof(processor->last_error) - 1);
    processor->last_error[sizeof(processor->last_error) - 1] = '\0';
    pthread_mutex_unlock(&processor->error_mutex);
    
    if (processor->config.error_callback) {
        processor->config.error_callback(error);
    }
}

/* 内存池实现 */
static linx_memory_pool_t *memory_pool_create(uint32_t size, uint32_t block_size)
{
    linx_memory_pool_t *pool = calloc(1, sizeof(linx_memory_pool_t));
    if (!pool) return NULL;
    
    pool->pool = calloc(size, sizeof(void*));
    if (!pool->pool) {
        free(pool);
        return NULL;
    }
    
    pool->size = size;
    pool->block_size = block_size;
    pool->used = 0;
    
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        free(pool->pool);
        free(pool);
        return NULL;
    }
    
    /* 预分配内存块 */
    for (uint32_t i = 0; i < size; i++) {
        void *block = aligned_alloc(LINX_MEMORY_ALIGNMENT, block_size);
        if (block) {
            pool->pool[i] = block;
        }
    }
    
    return pool;
}

static void memory_pool_destroy(linx_memory_pool_t *pool)
{
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    for (uint32_t i = 0; i < pool->size; i++) {
        if (pool->pool[i]) {
            free(pool->pool[i]);
        }
    }
    pthread_mutex_unlock(&pool->mutex);
    
    pthread_mutex_destroy(&pool->mutex);
    free(pool->pool);
    free(pool);
}

static void *memory_pool_alloc(linx_memory_pool_t *pool)
{
    if (!pool) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    void *block = NULL;
    
    if (pool->used < pool->size) {
        block = pool->pool[pool->used++];
        if (!block) {
            block = aligned_alloc(LINX_MEMORY_ALIGNMENT, pool->block_size);
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    return block;
}

static void memory_pool_free(linx_memory_pool_t *pool, void *block)
{
    if (!pool || !block) return;
    
    pthread_mutex_lock(&pool->mutex);
    if (pool->used > 0) {
        pool->pool[--pool->used] = block;
    } else {
        free(block);
    }
    pthread_mutex_unlock(&pool->mutex);
}

/* 事件队列实现 */
static linx_event_queue_t *event_queue_create(const linx_event_queue_config_t *config)
{
    linx_event_queue_t *queue = calloc(1, sizeof(linx_event_queue_t));
    if (!queue) return NULL;
    
    TAILQ_INIT(&queue->head);
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0 ||
        pthread_cond_init(&queue->not_empty, NULL) != 0 ||
        pthread_cond_init(&queue->not_full, NULL) != 0) {
        free(queue);
        return NULL;
    }
    
    queue->capacity = config->capacity;
    queue->high_watermark = config->high_watermark;
    queue->low_watermark = config->low_watermark;
    queue->drop_on_full = config->drop_on_full;
    queue->count = 0;
    
    return queue;
}

static void event_queue_destroy(linx_event_queue_t *queue)
{
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 清空队列 */
    linx_event_wrapper_t *wrapper, *tmp;
    TAILQ_FOREACH_SAFE(wrapper, &queue->head, queue_entry, tmp) {
        TAILQ_REMOVE(&queue->head, wrapper, queue_entry);
        free(wrapper);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    free(queue);
}

static int event_queue_enqueue(linx_event_queue_t *queue, linx_event_wrapper_t *wrapper)
{
    if (!queue || !wrapper) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 检查队列是否已满 */
    if (queue->count >= queue->capacity) {
        if (queue->drop_on_full) {
            queue->drop_count++;
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        } else {
            /* 等待队列有空位 */
            while (queue->count >= queue->capacity) {
                pthread_cond_wait(&queue->not_full, &queue->mutex);
            }
        }
    }
    
    /* 根据优先级插入队列 */
    if (wrapper->priority == LINX_EVENT_PRIORITY_CRITICAL || 
        wrapper->priority == LINX_EVENT_PRIORITY_HIGH) {
        /* 高优先级事件插入队头 */
        TAILQ_INSERT_HEAD(&queue->head, wrapper, queue_entry);
    } else {
        /* 普通优先级事件插入队尾 */
        TAILQ_INSERT_TAIL(&queue->head, wrapper, queue_entry);
    }
    
    queue->count++;
    queue->enqueue_count++;
    
    if (queue->count > queue->peak_size) {
        queue->peak_size = queue->count;
    }
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

static linx_event_wrapper_t *event_queue_dequeue(linx_event_queue_t *queue, uint32_t timeout_ms)
{
    if (!queue) return NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    struct timespec timeout;
    if (timeout_ms > 0) {
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += timeout_ms / 1000;
        timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }
    }
    
    /* 等待队列非空 */
    while (queue->count == 0) {
        int ret;
        if (timeout_ms > 0) {
            ret = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &timeout);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->mutex);
                return NULL;
            }
        } else {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }
    }
    
    /* 取出队头元素 */
    linx_event_wrapper_t *wrapper = TAILQ_FIRST(&queue->head);
    TAILQ_REMOVE(&queue->head, wrapper, queue_entry);
    queue->count--;
    queue->dequeue_count++;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return wrapper;
}

/* 事件获取工作函数 */
static void *event_fetch_worker(void *arg, int *should_stop)
{
    linx_task_arg_t *task_arg = (linx_task_arg_t *)arg;
    linx_event_processor_t *processor = task_arg->processor;
    linx_event_t *raw_event = NULL;
    int ret;
    
    LINX_LOG_DEBUG("Event fetch worker started");
    
    while (!*should_stop && !processor->shutdown_requested) {
        /* 从eBPF获取原始事件 */
        ret = linx_ebpf_get_ringbuf_msg(processor->ebpf_manager, &raw_event);
        if (ret != 0 || !raw_event) {
            usleep(1000);  /* 1ms */
            continue;
        }
        
        /* 分配事件包装器 */
        linx_event_wrapper_t *wrapper = memory_pool_alloc(processor->wrapper_pool);
        if (!wrapper) {
            pthread_mutex_lock(&processor->stats_mutex);
            processor->stats.allocation_failures++;
            processor->stats.total_events_dropped++;
            pthread_mutex_unlock(&processor->stats_mutex);
            continue;
        }
        
        /* 分配处理后的事件结构 */
        event_t *processed_event = memory_pool_alloc(processor->event_pool);
        if (!processed_event) {
            memory_pool_free(processor->wrapper_pool, wrapper);
            pthread_mutex_lock(&processor->stats_mutex);
            processor->stats.allocation_failures++;
            processor->stats.total_events_dropped++;
            pthread_mutex_unlock(&processor->stats_mutex);
            continue;
        }
        
        /* 事件丰富化处理 */
        memset(processed_event, 0, sizeof(event_t));
        
        /* 生成序列号 */
        pthread_mutex_lock(&processor->sequence_mutex);
        uint64_t seq_id = ++processor->sequence_counter;
        pthread_mutex_unlock(&processor->sequence_mutex);
        
        /* 填充基本信息 */
        processed_event->num = seq_id;
        snprintf(processed_event->time, sizeof(processed_event->time), 
                "%lu", get_timestamp_us());
        processed_event->type = "syscall";  /* TODO: 根据raw_event类型填充 */
        processed_event->rawres = 0;
        processed_event->failed = false;
        processed_event->dir[0] = '>';
        
        /* 设置包装器 */
        wrapper->event = processed_event;
        wrapper->timestamp_ns = get_timestamp_ns();
        wrapper->sequence_id = seq_id;
        wrapper->priority = LINX_EVENT_PRIORITY_NORMAL;
        wrapper->retry_count = 0;
        wrapper->context = processor;
        
        /* 提交匹配任务到匹配线程池 */
        linx_task_arg_t *match_task = malloc(sizeof(linx_task_arg_t));
        if (!match_task) {
            memory_pool_free(processor->event_pool, processed_event);
            memory_pool_free(processor->wrapper_pool, wrapper);
            pthread_mutex_lock(&processor->stats_mutex);
            processor->stats.allocation_failures++;
            processor->stats.total_events_dropped++;
            pthread_mutex_unlock(&processor->stats_mutex);
            continue;
        }
        
        match_task->type = LINX_TASK_TYPE_MATCH_EVENT;
        match_task->processor = processor;
        match_task->event_wrapper = wrapper;
        match_task->worker_id = task_arg->worker_id;
        
        ret = linx_thread_pool_add_task(processor->matcher_pool, 
                                       event_match_worker, match_task);
        if (ret != 0) {
            LINX_LOG_ERROR("Failed to submit match task to thread pool");
            free(match_task);
            memory_pool_free(processor->event_pool, processed_event);
            memory_pool_free(processor->wrapper_pool, wrapper);
            pthread_mutex_lock(&processor->stats_mutex);
            processor->stats.queue_full_events++;
            processor->stats.total_events_dropped++;
            pthread_mutex_unlock(&processor->stats_mutex);
            continue;
        }
        
        /* 更新统计 */
        pthread_mutex_lock(&processor->stats_mutex);
        processor->stats.total_events_received++;
        pthread_mutex_unlock(&processor->stats_mutex);
    }
    
    LINX_LOG_DEBUG("Event fetch worker stopped");
    return NULL;
}

/* 事件匹配工作函数 */
static void *event_match_worker(void *arg, int *should_stop)
{
    linx_task_arg_t *task_arg = (linx_task_arg_t *)arg;
    linx_event_processor_t *processor = task_arg->processor;
    linx_event_wrapper_t *wrapper = task_arg->event_wrapper;
    bool matched = false;
    
    if (!wrapper || !wrapper->event) {
        LINX_LOG_ERROR("Invalid event wrapper in match worker");
        free(task_arg);
        return NULL;
    }
    
    LINX_LOG_DEBUG("Event match worker processing event %lu", wrapper->event->num);
    
    /* 执行规则匹配 */
    linx_rule_set_t *rule_set = linx_rule_set_get();
    if (rule_set && rule_set->size > 0) {
        for (size_t i = 0; i < rule_set->size; i++) {
            if (rule_set->data.matches[i]) {
                /* TODO: 设置当前事件到线程本地存储 */
                matched = linx_rule_engine_match(rule_set->data.matches[i]);
                
                if (matched) {
                    /* 触发告警 */
                    if (rule_set->data.outputs[i]) {
                        char rule_name[64];
                        snprintf(rule_name, sizeof(rule_name), "rule_%zu", i);
                        linx_alert_send_async(rule_set->data.outputs[i], rule_name, 1);
                    }
                    
                    /* 更新匹配统计 */
                    pthread_mutex_lock(&processor->stats_mutex);
                    processor->stats.total_events_matched++;
                    pthread_mutex_unlock(&processor->stats_mutex);
                    
                    break;  /* 匹配第一个规则即停止 */
                }
            }
        }
    }
    
    /* 更新处理统计 */
    pthread_mutex_lock(&processor->stats_mutex);
    processor->stats.total_events_processed++;
    pthread_mutex_unlock(&processor->stats_mutex);
    
    /* 释放事件内存 */
    if (wrapper->event) {
        memory_pool_free(processor->event_pool, wrapper->event);
    }
    memory_pool_free(processor->wrapper_pool, wrapper);
    
    free(task_arg);
    return NULL;
}

/* 监控线程 */
static void *monitor_thread(void *arg)
{
    linx_event_processor_t *processor = (linx_event_processor_t *)arg;
    uint64_t last_stats_time = get_timestamp_us();
    uint64_t last_event_count = 0;
    
    set_thread_name("linx_monitor");
    
    while (processor->monitor_running) {
        sleep(1);
        
        uint64_t current_time = get_timestamp_us();
        uint64_t time_diff = current_time - last_stats_time;
        
        if (time_diff >= processor->config.monitor_config.stats_interval_ms * 1000) {
            pthread_mutex_lock(&processor->stats_mutex);
            
            /* 计算吞吐量 */
            uint64_t current_events = processor->stats.total_events_processed;
            uint64_t events_diff = current_events - last_event_count;
            
            if (time_diff > 0) {
                processor->stats.events_per_second = 
                    (events_diff * 1000000) / time_diff;
                
                if (processor->stats.events_per_second > 
                    processor->stats.peak_events_per_second) {
                    processor->stats.peak_events_per_second = 
                        processor->stats.events_per_second;
                }
            }
            
            /* 更新队列统计 */
            if (processor->processing_queue) {
                processor->stats.queue_current_size = processor->processing_queue->count;
                if (processor->processing_queue->peak_size > processor->stats.queue_peak_size) {
                    processor->stats.queue_peak_size = processor->processing_queue->peak_size;
                }
            }
            
            /* 更新线程统计 */
            if (processor->fetcher_pool) {
                processor->stats.fetcher_active_threads = 
                    linx_thread_pool_get_active_threads(processor->fetcher_pool);
                processor->stats.fetcher_queue_size = 
                    linx_thread_pool_get_queue_size(processor->fetcher_pool);
            }
            
            if (processor->matcher_pool) {
                processor->stats.matcher_active_threads = 
                    linx_thread_pool_get_active_threads(processor->matcher_pool);
                processor->stats.matcher_queue_size = 
                    linx_thread_pool_get_queue_size(processor->matcher_pool);
            }
            
            processor->stats.last_update_timestamp = current_time;
            processor->stats.uptime_seconds = (current_time - processor->profiling.start_time) / 1000000;
            
            pthread_mutex_unlock(&processor->stats_mutex);
            
            last_stats_time = current_time;
            last_event_count = current_events;
            
            /* 调用统计回调 */
            if (processor->config.stats_callback) {
                processor->config.stats_callback(&processor->stats);
            }
        }
    }
    
    return NULL;
}

/* 公共API实现 */

void linx_event_processor_get_default_config(linx_event_processor_config_t *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(linx_event_processor_config_t));
    
    uint32_t cpu_count = get_cpu_count();
    
    /* 核心配置 */
    config->fetcher_thread_count = cpu_count;
    config->matcher_thread_count = cpu_count * 2;
    
    /* 队列配置 */
    config->queue_config.capacity = LINX_EVENT_PROCESSOR_DEFAULT_QUEUE_SIZE;
    config->queue_config.high_watermark = config->queue_config.capacity * 8 / 10;
    config->queue_config.low_watermark = config->queue_config.capacity * 2 / 10;
    config->queue_config.drop_on_full = false;
    config->queue_config.batch_size = LINX_EVENT_PROCESSOR_DEFAULT_BATCH_SIZE;
    
    /* 获取线程池配置 */
    config->fetcher_pool_config.thread_count = cpu_count;
    config->fetcher_pool_config.cpu_affinity = true;
    config->fetcher_pool_config.priority = 0;
    
    /* 匹配线程池配置 */
    config->matcher_pool_config.thread_count = cpu_count * 2;
    config->matcher_pool_config.cpu_affinity = true;
    config->matcher_pool_config.priority = 0;
    
    /* 性能配置 */
    config->perf_config.enable_prefetch = true;
    config->perf_config.enable_batch_processing = true;
    config->perf_config.enable_zero_copy = false;
    config->perf_config.spin_lock_timeout_us = 100;
    config->perf_config.memory_pool_size = LINX_EVENT_PROCESSOR_DEFAULT_MEMORY_POOL_SIZE;
    
    /* 监控配置 */
    config->monitor_config.enable_metrics = true;
    config->monitor_config.stats_interval_ms = 1000;
    config->monitor_config.enable_latency_tracking = true;
    config->monitor_config.enable_cpu_profiling = false;
}

int linx_event_processor_validate_config(const linx_event_processor_config_t *config)
{
    if (!config) return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    
    if (config->fetcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||
        config->fetcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS) {
        return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    }
    
    if (config->matcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||
        config->matcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS) {
        return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    }
    
    if (config->queue_config.capacity < LINX_EVENT_PROCESSOR_MIN_QUEUE_SIZE ||
        config->queue_config.capacity > LINX_EVENT_PROCESSOR_MAX_QUEUE_SIZE) {
        return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    }
    
    return LINX_EVENT_PROC_SUCCESS;
}

linx_event_processor_t *linx_event_processor_create(const linx_event_processor_config_t *config)
{
    linx_event_processor_t *processor = calloc(1, sizeof(linx_event_processor_t));
    if (!processor) return NULL;
    
    /* 使用默认配置或用户配置 */
    if (config) {
        if (linx_event_processor_validate_config(config) != 0) {
            free(processor);
            return NULL;
        }
        processor->config = *config;
    } else {
        linx_event_processor_get_default_config(&processor->config);
    }
    
    /* 初始化状态 */
    processor->state = LINX_EVENT_PROC_STOPPED;
    processor->sequence_counter = 0;
    processor->shutdown_requested = false;
    
    /* 初始化互斥锁 */
    if (pthread_mutex_init(&processor->state_mutex, NULL) != 0 ||
        pthread_cond_init(&processor->state_cond, NULL) != 0 ||
        pthread_mutex_init(&processor->stats_mutex, NULL) != 0 ||
        pthread_mutex_init(&processor->sequence_mutex, NULL) != 0 ||
        pthread_mutex_init(&processor->error_mutex, NULL) != 0 ||
        pthread_mutex_init(&processor->config_mutex, NULL) != 0) {
        goto error_cleanup;
    }
    
    /* 创建内存池 */
    processor->event_pool = memory_pool_create(
        processor->config.perf_config.memory_pool_size,
        sizeof(event_t));
    if (!processor->event_pool) goto error_cleanup;
    
    processor->wrapper_pool = memory_pool_create(
        processor->config.perf_config.memory_pool_size,
        sizeof(linx_event_wrapper_t));
    if (!processor->wrapper_pool) goto error_cleanup;
    
    /* 创建事件队列 */
    processor->processing_queue = event_queue_create(&processor->config.queue_config);
    if (!processor->processing_queue) goto error_cleanup;
    
    /* 创建线程池 */
    processor->fetcher_pool = linx_thread_pool_create(
        processor->config.fetcher_pool_config.thread_count);
    if (!processor->fetcher_pool) goto error_cleanup;
    
    processor->matcher_pool = linx_thread_pool_create(
        processor->config.matcher_pool_config.thread_count);
    if (!processor->matcher_pool) goto error_cleanup;
    
    LINX_LOG_INFO("Event processor created successfully");
    return processor;
    
error_cleanup:
    linx_event_processor_destroy(processor);
    return NULL;
}

void linx_event_processor_destroy(linx_event_processor_t *processor)
{
    if (!processor) return;
    
    /* 确保处理器已停止 */
    if (processor->state != LINX_EVENT_PROC_STOPPED) {
        linx_event_processor_stop(processor, 5000);
    }
    
    /* 销毁线程池 */
    if (processor->fetcher_pool) {
        linx_thread_pool_destroy(processor->fetcher_pool, 1);
    }
    if (processor->matcher_pool) {
        linx_thread_pool_destroy(processor->matcher_pool, 1);
    }
    
    /* 销毁队列 */
    if (processor->processing_queue) {
        event_queue_destroy(processor->processing_queue);
    }
    
    /* 销毁内存池 */
    if (processor->event_pool) {
        memory_pool_destroy(processor->event_pool);
    }
    if (processor->wrapper_pool) {
        memory_pool_destroy(processor->wrapper_pool);
    }
    
    /* 销毁互斥锁 */
    pthread_mutex_destroy(&processor->state_mutex);
    pthread_cond_destroy(&processor->state_cond);
    pthread_mutex_destroy(&processor->stats_mutex);
    pthread_mutex_destroy(&processor->sequence_mutex);
    pthread_mutex_destroy(&processor->error_mutex);
    pthread_mutex_destroy(&processor->config_mutex);
    
    free(processor);
    LINX_LOG_INFO("Event processor destroyed");
}

int linx_event_processor_start(linx_event_processor_t *processor, linx_ebpf_t *ebpf_manager)
{
    if (!processor || !ebpf_manager) {
        return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&processor->state_mutex);
    
    if (processor->state != LINX_EVENT_PROC_STOPPED) {
        pthread_mutex_unlock(&processor->state_mutex);
        return LINX_EVENT_PROC_ERROR_ALREADY_RUNNING;
    }
    
    processor->state = LINX_EVENT_PROC_STARTING;
    processor->ebpf_manager = ebpf_manager;
    processor->shutdown_requested = false;
    
    pthread_mutex_unlock(&processor->state_mutex);
    
    /* 为每个获取线程提交持续运行任务 */
    for (uint32_t i = 0; i < processor->config.fetcher_thread_count; i++) {
        linx_task_arg_t *task_arg = malloc(sizeof(linx_task_arg_t));
        if (!task_arg) {
            set_last_error(processor, "Failed to allocate memory for fetch task");
            processor->state = LINX_EVENT_PROC_ERROR;
            return LINX_EVENT_PROC_ERROR_OUT_OF_MEMORY;
        }
        
        task_arg->type = LINX_TASK_TYPE_FETCH_EVENT;
        task_arg->processor = processor;
        task_arg->event_wrapper = NULL;
        task_arg->worker_id = i;
        
        int ret = linx_thread_pool_add_task(processor->fetcher_pool, 
                                           event_fetch_worker, task_arg);
        if (ret != 0) {
            free(task_arg);
            set_last_error(processor, "Failed to submit fetch task to thread pool");
            processor->state = LINX_EVENT_PROC_ERROR;
            return LINX_EVENT_PROC_ERROR_THREAD_CREATE;
        }
    }
    
    /* 启动监控线程 */
    if (processor->config.monitor_config.enable_metrics) {
        processor->monitor_running = true;
        if (pthread_create(&processor->monitor_thread, NULL, monitor_thread, processor) != 0) {
            set_last_error(processor, "Failed to create monitor thread");
            processor->state = LINX_EVENT_PROC_ERROR;
            return LINX_EVENT_PROC_ERROR_THREAD_CREATE;
        }
    }
    
    /* 设置状态为运行中 */
    pthread_mutex_lock(&processor->state_mutex);
    processor->state = LINX_EVENT_PROC_RUNNING;
    processor->stats.uptime_seconds = 0;
    processor->profiling.start_time = get_timestamp_ns();
    pthread_cond_broadcast(&processor->state_cond);
    pthread_mutex_unlock(&processor->state_mutex);
    
    LINX_LOG_INFO("Event processor started successfully");
    return LINX_EVENT_PROC_SUCCESS;
}

int linx_event_processor_stop(linx_event_processor_t *processor, uint32_t timeout_ms)
{
    if (!processor) return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&processor->state_mutex);
    
    if (processor->state != LINX_EVENT_PROC_RUNNING) {
        pthread_mutex_unlock(&processor->state_mutex);
        return LINX_EVENT_PROC_ERROR_NOT_RUNNING;
    }
    
    processor->state = LINX_EVENT_PROC_STOPPING;
    processor->shutdown_requested = true;
    pthread_mutex_unlock(&processor->state_mutex);
    
    LINX_LOG_INFO("Stopping event processor...");
    
    /* 停止监控线程 */
    if (processor->monitor_running) {
        processor->monitor_running = false;
        pthread_join(processor->monitor_thread, NULL);
    }
    
    /* 停止线程池 */
    if (processor->fetcher_pool) {
        linx_thread_pool_destroy(processor->fetcher_pool, 1);
        processor->fetcher_pool = NULL;
    }
    
    if (processor->matcher_pool) {
        linx_thread_pool_destroy(processor->matcher_pool, 1);
        processor->matcher_pool = NULL;
    }
    
    /* 重新创建线程池以备下次使用 */
    processor->fetcher_pool = linx_thread_pool_create(
        processor->config.fetcher_pool_config.thread_count);
    processor->matcher_pool = linx_thread_pool_create(
        processor->config.matcher_pool_config.thread_count);
    
    /* 设置状态为已停止 */
    pthread_mutex_lock(&processor->state_mutex);
    processor->state = LINX_EVENT_PROC_STOPPED;
    processor->shutdown_requested = false;
    pthread_cond_broadcast(&processor->state_cond);
    pthread_mutex_unlock(&processor->state_mutex);
    
    LINX_LOG_INFO("Event processor stopped");
    return LINX_EVENT_PROC_SUCCESS;
}

linx_event_processor_state_t linx_event_processor_get_state(linx_event_processor_t *processor)
{
    if (!processor) return LINX_EVENT_PROC_ERROR;
    
    pthread_mutex_lock(&processor->state_mutex);
    linx_event_processor_state_t state = processor->state;
    pthread_mutex_unlock(&processor->state_mutex);
    
    return state;
}

int linx_event_processor_get_stats(linx_event_processor_t *processor, 
                                  linx_event_processor_stats_t *stats)
{
    if (!processor || !stats) return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&processor->stats_mutex);
    *stats = processor->stats;
    pthread_mutex_unlock(&processor->stats_mutex);
    
    return LINX_EVENT_PROC_SUCCESS;
}

int linx_event_processor_reset_stats(linx_event_processor_t *processor)
{
    if (!processor) return LINX_EVENT_PROC_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&processor->stats_mutex);
    memset(&processor->stats, 0, sizeof(processor->stats));
    processor->stats.last_update_timestamp = get_timestamp_us();
    processor->profiling.start_time = get_timestamp_ns();
    pthread_mutex_unlock(&processor->stats_mutex);
    
    return LINX_EVENT_PROC_SUCCESS;
}

const char *linx_event_processor_get_last_error(linx_event_processor_t *processor)
{
    if (!processor) return "Invalid processor";
    
    pthread_mutex_lock(&processor->error_mutex);
    const char *error = processor->last_error;
    pthread_mutex_unlock(&processor->error_mutex);
    
    return error;
}

/* 高级功能的基本实现 */
int linx_event_processor_pause(linx_event_processor_t *processor)
{
    /* TODO: 实现暂停功能 */
    return LINX_EVENT_PROC_SUCCESS;
}

int linx_event_processor_resume(linx_event_processor_t *processor)
{
    /* TODO: 实现恢复功能 */
    return LINX_EVENT_PROC_SUCCESS;
}

int linx_event_processor_flush(linx_event_processor_t *processor, uint32_t timeout_ms)
{
    /* TODO: 实现队列刷新功能 */
    return LINX_EVENT_PROC_SUCCESS;
}

uint64_t linx_event_processor_gc(linx_event_processor_t *processor)
{
    /* TODO: 实现垃圾回收功能 */
    return 0;
}

int linx_event_processor_dump_state(linx_event_processor_t *processor, 
                                   char *buffer, size_t buffer_size)
{
    /* TODO: 实现状态导出功能 */
    return 0;
}

int linx_event_processor_set_profiling(linx_event_processor_t *processor, bool enable)
{
    /* TODO: 实现性能分析控制 */
    return LINX_EVENT_PROC_SUCCESS;
}

int linx_event_processor_update_config(linx_event_processor_t *processor, 
                                      const linx_event_processor_config_t *config)
{
    /* TODO: 实现热配置更新 */
    return LINX_EVENT_PROC_SUCCESS;
}
