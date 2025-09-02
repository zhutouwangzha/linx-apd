#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "rule_match_mt.h"
#include "linx_rule_engine_set.h"
#include "linx_hash_map.h"
#include "linx_log.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_alert.h"
#include "linx_event_rich.h"

/* 全局多线程管理器 */
static rule_match_mt_manager_t *g_mt_manager = NULL;

/* 线程本地存储键 - 用于存储事件上下文 */
static pthread_key_t g_context_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

/* 事件上下文结构 */
typedef struct {
    event_t evt;                       /* 事件数据副本 */
    field_update_table_t tables[5];    /* 字段更新表 */
    void *fd_info;                     /* fd信息 */
    void *proc_info;                   /* 进程信息 */
    void *user_info;                   /* 用户信息 */
    void *group_info;                  /* 组信息 */
} thread_event_context_t;

/* 创建线程本地存储键 */
static void create_context_key(void)
{
    pthread_key_create(&g_context_key, free);
}

/* 创建线程事件上下文 */
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

/* 规则匹配工作线程函数 */
static void *rule_match_worker(void *arg, int *should_stop)
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
    ctx = pthread_getspecific(g_context_key);
    if (!ctx) {
        ctx = create_thread_context(task->event, task->fd);
        if (!ctx) {
            LINX_LOG_ERROR("Failed to create thread context");
            goto cleanup;
        }
        pthread_setspecific(g_context_key, ctx);
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

/* 初始化多线程规则匹配 */
int linx_rule_match_mt_init(int num_threads)
{
    if (g_mt_manager) {
        LINX_LOG_WARNING("Multi-thread rule match already initialized");
        return 0;
    }
    
    g_mt_manager = calloc(1, sizeof(rule_match_mt_manager_t));
    if (!g_mt_manager) {
        return -1;
    }
    
    /* 创建线程池 */
    g_mt_manager->thread_pool = linx_thread_pool_create(num_threads);
    if (!g_mt_manager->thread_pool) {
        free(g_mt_manager);
        g_mt_manager = NULL;
        return -1;
    }
    
    /* 创建线程本地存储键 */
    pthread_once(&g_key_once, create_context_key);
    
    g_mt_manager->initialized = true;
    
    LINX_LOG_INFO("Initialized multi-thread rule match with %d threads", num_threads);
    
    return 0;
}

/* 清理多线程规则匹配 */
void linx_rule_match_mt_deinit(void)
{
    if (!g_mt_manager) {
        return;
    }
    
    if (g_mt_manager->thread_pool) {
        linx_thread_pool_destroy(g_mt_manager->thread_pool, 1);
    }
    
    free(g_mt_manager);
    g_mt_manager = NULL;
    
    LINX_LOG_INFO("Deinitialized multi-thread rule match");
}

/* 多线程规则匹配 */
bool linx_rule_set_match_rule_mt(linx_event_t *event, int64_t fd)
{
    linx_rule_set_t *rule_set = linx_rule_set_get();
    bool match_result = false;
    pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t complete_cond = PTHREAD_COND_INITIALIZER;
    int completed_count = 0;
    int total_tasks = 0;
    int ret;
    
    if (!g_mt_manager || !g_mt_manager->initialized || !rule_set || rule_set->size == 0) {
        /* 回退到单线程模式 */
        return linx_rule_set_match_rule();
    }
    
    /* 获取活跃线程数 */
    int num_threads = linx_thread_pool_get_active_threads(g_mt_manager->thread_pool);
    if (num_threads <= 0) {
        num_threads = g_mt_manager->thread_pool->thread_count;
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
        ret = linx_thread_pool_add_task(g_mt_manager->thread_pool, rule_match_worker, task);
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