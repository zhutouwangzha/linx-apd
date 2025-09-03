#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <pthread.h>

#include "linx_event_processor.h"
#include "linx_event_processor_task.h"
#include "linx_log.h"
#include "linx_event.h"
#include "linx_rule_engine_set.h"
#include "linx_engine.h"
#include "linx_hash_map.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_alert.h"
#include "linx_event_rich.h"

static linx_event_processor_t *g_event_processor = NULL;

/* 创建线程本地存储键 */
static void create_context_key(void)
{
    if (g_event_processor) {
        pthread_key_create(&g_event_processor->context_key, free);
    }
}

/* 创建线程事件上下文（从rule_match_mt移植） */
static thread_event_context_t *create_thread_context(linx_event_t *event, int64_t fd)
{
    thread_event_context_t *ctx = calloc(1, sizeof(thread_event_context_t));
    if (!ctx) {
        return NULL;
    }
    
    /* 复制当前事件数据 */
    memcpy(&ctx->evt, linx_event_rich_get(), sizeof(event_t));
    
    /* 获取各种信息的副本 */
    ctx->fd_info = linx_process_cache_get_fd((pid_t)event->pid, fd);
    ctx->proc_info = linx_process_cache_get((pid_t)event->pid);
    ctx->user_info = linx_machine_status_get_user();
    ctx->group_info = linx_machine_status_get_group();
    
    /* 设置字段更新表 */
    ctx->tables[0].table_name = "evt";
    ctx->tables[0].base_addr = &ctx->evt;
    
    ctx->tables[1].table_name = "fd";
    ctx->tables[1].base_addr = ctx->fd_info;
    
    ctx->tables[2].table_name = "proc";
    ctx->tables[2].base_addr = ctx->proc_info;
    
    ctx->tables[3].table_name = "user";
    ctx->tables[3].base_addr = ctx->user_info;
    
    ctx->tables[4].table_name = "group";
    ctx->tables[4].base_addr = ctx->group_info;
    
    return ctx;
}

static uint32_t get_cpu_count(void)
{
    return (uint32_t)get_nprocs();
}

static int linx_event_processor_validate_config(linx_event_processor_config_t *config)
{
    if (!config) {
        return -1;
    }

    if (config->fetcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||
        config->fetcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS)
    {
        return -1;
    }

    if (config->matcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||
        config->matcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS)
    {
        return -1;
    }

    return 0;
}

static void linx_event_processor_get_default_config(linx_event_processor_config_t *config)
{
    uint32_t cpu_count;

    if (!config) {
        return;
    }

    memset(config, 0, sizeof(linx_event_processor_config_t));

    cpu_count = get_cpu_count();

    config->fetcher_thread_count = 1;  /* 不需要独立的fetcher线程 */
    config->matcher_thread_count = cpu_count;  /* 使用CPU核心数进行匹配 */
}

/* 规则匹配工作线程函数（融合rule_match_mt逻辑） */
static void *event_match_worker(void *arg, int *should_stop)
{
    rule_match_task_arg_t *task = (rule_match_task_arg_t *)arg;
    thread_event_context_t *ctx = NULL;
    linx_rule_set_t *rule_set = linx_rule_set_get();
    bool local_match = false;
    
    /* 检查是否需要停止 */
    if (*should_stop == 2) {
        goto cleanup;
    }
    
    if (!task || !rule_set) {
        goto cleanup;
    }
    
    /* 获取或创建线程本地事件上下文 */
    ctx = pthread_getspecific(g_event_processor->context_key);
    if (!ctx) {
        ctx = create_thread_context(task->event, task->fd);
        if (!ctx) {
            LINX_LOG_ERROR("Failed to create thread context");
            goto cleanup;
        }
        pthread_setspecific(g_event_processor->context_key, ctx);
    } else {
        /* 更新现有上下文 */
        memcpy(&ctx->evt, linx_event_rich_get(), sizeof(event_t));
        ctx->fd_info = linx_process_cache_get_fd((pid_t)task->event->pid, task->fd);
        ctx->proc_info = linx_process_cache_get((pid_t)task->event->pid);
    }
    
    /* 更新当前线程的基地址 */
    linx_hash_map_update_tables_base(ctx->tables, 5);
    
    /* 遍历分配的规则范围 */
    for (size_t i = task->rule_start; i < task->rule_end && i < rule_set->size; i++) {
        if (*should_stop) {
            break;
        }
        
        if (rule_set->data.matches[i]) {
            if (rule_set->data.matches[i]->func(rule_set->data.matches[i]->context)) {
                local_match = true;
                
                /* 发送告警 */
                linx_alert_send_async(rule_set->data.outputs[i], rule_set->data.rules[i]);
                
                /* TODO: 根据配置决定是否继续匹配 */
                break;
            }
        }
    }
    
    /* 更新共享结果 */
    if (local_match) {
        pthread_mutex_lock(task->result_mutex);
        *(task->match_result) = true;
        pthread_mutex_unlock(task->result_mutex);
    }
    
cleanup:
    /* 通知主线程任务完成 */
    if (task) {
        pthread_mutex_lock(task->result_mutex);
        (*task->completed_count)++;
        pthread_cond_signal(task->complete_cond);
        pthread_mutex_unlock(task->result_mutex);
        
        free(task);
    }
    
    return NULL;
}

/* 注意：event_fetch_worker 已移除，因为我们现在专注于规则匹配
 * 事件获取由主线程处理，事件处理器只负责多线程规则匹配 */

int linx_event_processor_init(linx_event_processor_config_t *config)
{
    if (g_event_processor) {
        return 0;
    }

    g_event_processor = calloc(1, sizeof(linx_event_processor_t));
    if (!g_event_processor) {
        return -1;
    }

    if (config) {
        if (linx_event_processor_validate_config(config)) {
            free(g_event_processor);
            return -1;
        }

        g_event_processor->config = *config;
    } else {
        linx_event_processor_get_default_config(&g_event_processor->config);
    }

    /* 只创建matcher_pool用于规则匹配，不需要fetcher_pool */
    g_event_processor->fetcher_pool = NULL;  /* 保留字段用于未来扩展 */
    
    g_event_processor->matcher_pool = linx_thread_pool_create(g_event_processor->config.matcher_thread_count);
    if (!g_event_processor->matcher_pool) {
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    /* 初始化线程本地存储 */
    g_event_processor->key_once = PTHREAD_ONCE_INIT;
    pthread_once(&g_event_processor->key_once, create_context_key);
    
    g_event_processor->initialized = true;
    
    LINX_LOG_INFO("Initialized event processor with %d matcher threads for rule matching",
                  g_event_processor->config.matcher_thread_count);

    return 0;
}

void linx_event_processor_deinit(void)
{
    if (!g_event_processor) {
        return;
    }

    /* 销毁线程池 */
    if (g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);
    }
    
    /* fetcher_pool现在为NULL，不需要销毁 */

    free(g_event_processor);
    g_event_processor = NULL;
    
    LINX_LOG_INFO("Event processor deinitialized");
}

int linx_event_processor_start(void)
{
    if (!g_event_processor) {
        return -1;
    }

    /* 事件处理器现在专注于规则匹配，不需要独立的fetcher线程 */
    /* fetcher_pool 保留用于未来扩展，matcher_pool 用于规则匹配 */
    
    LINX_LOG_INFO("Event processor started - ready for rule matching");
    return 0;
}

int linx_event_processor_stop(void)
{
    if (!g_event_processor) {
        return -1;
    }

    /* 停止matcher线程池 */
    if (g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);
        g_event_processor->matcher_pool = NULL;
    }
    
    g_event_processor->initialized = false;
    
    LINX_LOG_INFO("Event processor stopped");
    return 0;
}

/* 多线程规则匹配（融合rule_match_mt逻辑） */
bool linx_event_processor_process_event(linx_event_t *event, int64_t fd)
{
    linx_rule_set_t *rule_set = linx_rule_set_get();
    bool match_result = false;
    pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t complete_cond = PTHREAD_COND_INITIALIZER;
    int completed_count = 0;
    int total_tasks = 0;
    int ret;
    
    if (!g_event_processor || !g_event_processor->initialized || !rule_set || rule_set->size == 0) {
        /* 回退到单线程模式 */
        return linx_rule_set_match_rule();
    }
    
    /* 获取活跃线程数 */
    int num_threads = linx_thread_pool_get_active_threads(g_event_processor->matcher_pool);
    if (num_threads <= 0) {
        num_threads = g_event_processor->matcher_pool->thread_count;
    }
    
    /* 计算每个线程处理的规则数 */
    size_t rules_per_thread = rule_set->size / num_threads;
    if (rules_per_thread == 0) {
        rules_per_thread = 1;
        num_threads = rule_set->size;
    }
    
    /* 创建并提交任务 */
    size_t current_idx = 0;
    for (int i = 0; i < num_threads && current_idx < rule_set->size; i++) {
        rule_match_task_arg_t *task = calloc(1, sizeof(rule_match_task_arg_t));
        if (!task) {
            LINX_LOG_ERROR("Failed to allocate task memory");
            continue;
        }
        
        task->event = event;
        task->fd = fd;
        task->rule_start = current_idx;
        task->rule_end = current_idx + rules_per_thread;
        
        /* 最后一个线程处理剩余的规则 */
        if (i == num_threads - 1) {
            task->rule_end = rule_set->size;
        }
        
        task->match_result = &match_result;
        task->result_mutex = &result_mutex;
        task->complete_cond = &complete_cond;
        task->completed_count = &completed_count;
        
        /* 提交任务到线程池 */
        ret = linx_thread_pool_add_task(g_event_processor->matcher_pool, event_match_worker, task);
        if (ret != 0) {
            LINX_LOG_ERROR("Failed to add task to thread pool");
            free(task);
        } else {
            total_tasks++;
        }
        
        current_idx = task->rule_end;
    }
    
    /* 等待所有任务完成 */
    if (total_tasks > 0) {
        pthread_mutex_lock(&result_mutex);
        while (completed_count < total_tasks) {
            pthread_cond_wait(&complete_cond, &result_mutex);
        }
        pthread_mutex_unlock(&result_mutex);
    }
    
    /* 清理资源 */
    pthread_mutex_destroy(&result_mutex);
    pthread_cond_destroy(&complete_cond);
    
    return match_result;
}

linx_event_processor_t *linx_event_processor_get(void)
{
    return g_event_processor;
}
