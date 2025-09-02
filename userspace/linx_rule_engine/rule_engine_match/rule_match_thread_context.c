#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "rule_match_thread_context.h"
#include "linx_rule_engine_set.h"
#include "linx_hash_map.h"
#include "linx_log.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_alert.h"

/* 线程池结构 */
typedef struct {
    pthread_t *threads;              /* 线程数组 */
    int num_threads;                 /* 线程数量 */
    pthread_mutex_t queue_mutex;     /* 队列互斥锁 */
    pthread_cond_t queue_cond;       /* 队列条件变量 */
    rule_match_task_t **task_queue;  /* 任务队列 */
    int queue_size;                  /* 队列大小 */
    int queue_head;                  /* 队列头 */
    int queue_tail;                  /* 队列尾 */
    int queue_count;                 /* 队列中任务数 */
    bool shutdown;                   /* 关闭标志 */
} thread_pool_t;

static thread_pool_t *g_thread_pool = NULL;

/* 线程本地存储键 */
static pthread_key_t g_context_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

/* 创建线程本地存储键 */
static void create_context_key(void)
{
    pthread_key_create(&g_context_key, NULL);
}

/* 工作线程函数 */
static void *worker_thread(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    rule_match_task_t *task;
    thread_event_context_t *context = NULL;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        /* 等待任务或关闭信号 */
        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }
        
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        /* 从队列中取出任务 */
        task = pool->task_queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_size;
        pool->queue_count--;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        /* 执行任务 */
        if (task) {
            /* 创建线程本地事件上下文 */
            context = linx_create_thread_event_context(task->event, task->fd);
            if (context) {
                /* 设置线程本地存储 */
                pthread_setspecific(g_context_key, context);
                
                /* 在当前上下文中匹配规则 */
                bool match = linx_match_rules_in_context(context, task->rule_start, task->rule_end);
                
                /* 更新结果 */
                if (match) {
                    pthread_mutex_lock(task->result_mutex);
                    *(task->match_result) = true;
                    pthread_mutex_unlock(task->result_mutex);
                }
                
                /* 清理上下文 */
                linx_destroy_thread_event_context(context);
                pthread_setspecific(g_context_key, NULL);
            }
            
            free(task);
        }
    }
    
    return NULL;
}

/* 初始化线程池 */
int linx_rule_match_thread_pool_init(thread_pool_config_t *config)
{
    if (g_thread_pool) {
        LINX_LOG_WARNING("Thread pool already initialized");
        return -1;
    }
    
    g_thread_pool = calloc(1, sizeof(thread_pool_t));
    if (!g_thread_pool) {
        return -1;
    }
    
    g_thread_pool->num_threads = config->num_threads;
    g_thread_pool->queue_size = config->queue_size;
    g_thread_pool->shutdown = false;
    
    /* 初始化互斥锁和条件变量 */
    pthread_mutex_init(&g_thread_pool->queue_mutex, NULL);
    pthread_cond_init(&g_thread_pool->queue_cond, NULL);
    
    /* 创建任务队列 */
    g_thread_pool->task_queue = calloc(config->queue_size, sizeof(rule_match_task_t *));
    if (!g_thread_pool->task_queue) {
        free(g_thread_pool);
        g_thread_pool = NULL;
        return -1;
    }
    
    /* 创建工作线程 */
    g_thread_pool->threads = calloc(config->num_threads, sizeof(pthread_t));
    if (!g_thread_pool->threads) {
        free(g_thread_pool->task_queue);
        free(g_thread_pool);
        g_thread_pool = NULL;
        return -1;
    }
    
    /* 创建线程本地存储键 */
    pthread_once(&g_key_once, create_context_key);
    
    /* 启动工作线程 */
    for (int i = 0; i < config->num_threads; i++) {
        if (pthread_create(&g_thread_pool->threads[i], NULL, worker_thread, g_thread_pool) != 0) {
            /* 清理已创建的线程 */
            g_thread_pool->shutdown = true;
            pthread_cond_broadcast(&g_thread_pool->queue_cond);
            for (int j = 0; j < i; j++) {
                pthread_join(g_thread_pool->threads[j], NULL);
            }
            free(g_thread_pool->threads);
            free(g_thread_pool->task_queue);
            free(g_thread_pool);
            g_thread_pool = NULL;
            return -1;
        }
    }
    
    return 0;
}

/* 销毁线程池 */
void linx_rule_match_thread_pool_destroy(void)
{
    if (!g_thread_pool) {
        return;
    }
    
    /* 设置关闭标志 */
    pthread_mutex_lock(&g_thread_pool->queue_mutex);
    g_thread_pool->shutdown = true;
    pthread_cond_broadcast(&g_thread_pool->queue_cond);
    pthread_mutex_unlock(&g_thread_pool->queue_mutex);
    
    /* 等待所有线程结束 */
    for (int i = 0; i < g_thread_pool->num_threads; i++) {
        pthread_join(g_thread_pool->threads[i], NULL);
    }
    
    /* 清理资源 */
    pthread_mutex_destroy(&g_thread_pool->queue_mutex);
    pthread_cond_destroy(&g_thread_pool->queue_cond);
    free(g_thread_pool->threads);
    free(g_thread_pool->task_queue);
    free(g_thread_pool);
    g_thread_pool = NULL;
}

/* 创建线程本地事件上下文 */
thread_event_context_t *linx_create_thread_event_context(linx_event_t *event, int64_t fd)
{
    thread_event_context_t *context = calloc(1, sizeof(thread_event_context_t));
    if (!context) {
        return NULL;
    }
    
    /* 复制事件数据 */
    memcpy(&context->evt, linx_event_rich_get(), sizeof(event_t));
    
    /* 获取并缓存各种信息 */
    context->fd_cache = linx_process_cache_get_fd((pid_t)event->pid, fd);
    context->proc_cache = linx_process_cache_get((pid_t)event->pid);
    context->user_cache = linx_machine_status_get_user();
    context->group_cache = linx_machine_status_get_group();
    
    /* 创建字段更新表 */
    context->table_count = 5;
    context->tables = calloc(context->table_count, sizeof(field_update_table_t));
    if (!context->tables) {
        free(context);
        return NULL;
    }
    
    /* 设置表项 */
    context->tables[0].table_name = "evt";
    context->tables[0].base_addr = &context->evt;
    
    context->tables[1].table_name = "fd";
    context->tables[1].base_addr = context->fd_cache;
    
    context->tables[2].table_name = "proc";
    context->tables[2].base_addr = context->proc_cache;
    
    context->tables[3].table_name = "user";
    context->tables[3].base_addr = context->user_cache;
    
    context->tables[4].table_name = "group";
    context->tables[4].base_addr = context->group_cache;
    
    return context;
}

/* 销毁线程本地事件上下文 */
void linx_destroy_thread_event_context(thread_event_context_t *context)
{
    if (!context) {
        return;
    }
    
    free(context->tables);
    free(context);
}

/* 在指定的事件上下文中匹配规则 */
bool linx_match_rules_in_context(thread_event_context_t *context, size_t start_idx, size_t end_idx)
{
    linx_rule_set_t *rule_set = linx_rule_set_get();
    bool match = false;
    
    if (!rule_set || !context) {
        return false;
    }
    
    /* 更新当前线程的基地址 */
    linx_hash_map_update_tables_base(context->tables, context->table_count);
    
    /* 遍历指定范围的规则 */
    for (size_t i = start_idx; i < end_idx && i < rule_set->size; i++) {
        if (rule_set->data.matches[i]) {
            if (rule_set->data.matches[i]->func(rule_set->data.matches[i]->context)) {
                match = true;
                
                /* 发送告警 */
                linx_alert_send_async(rule_set->data.outputs[i], rule_set->data.rules[i]);
                
                /* 这里可以根据配置决定是否继续匹配 */
                break;
            }
        }
    }
    
    return match;
}

/* 提交任务到线程池 */
static int submit_task_to_pool(rule_match_task_t *task)
{
    if (!g_thread_pool || !task) {
        return -1;
    }
    
    pthread_mutex_lock(&g_thread_pool->queue_mutex);
    
    /* 检查队列是否已满 */
    if (g_thread_pool->queue_count >= g_thread_pool->queue_size) {
        pthread_mutex_unlock(&g_thread_pool->queue_mutex);
        return -1;
    }
    
    /* 添加任务到队列 */
    g_thread_pool->task_queue[g_thread_pool->queue_tail] = task;
    g_thread_pool->queue_tail = (g_thread_pool->queue_tail + 1) % g_thread_pool->queue_size;
    g_thread_pool->queue_count++;
    
    /* 通知等待的线程 */
    pthread_cond_signal(&g_thread_pool->queue_cond);
    
    pthread_mutex_unlock(&g_thread_pool->queue_mutex);
    
    return 0;
}

/* 多线程规则匹配 */
bool linx_rule_set_match_rule_mt(linx_event_t *event, int64_t fd)
{
    linx_rule_set_t *rule_set = linx_rule_set_get();
    bool match_result = false;
    pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    if (!rule_set || !g_thread_pool || rule_set->size == 0) {
        return false;
    }
    
    /* 计算每个线程处理的规则数量 */
    size_t rules_per_thread = rule_set->size / g_thread_pool->num_threads;
    size_t remaining_rules = rule_set->size % g_thread_pool->num_threads;
    
    /* 创建并提交任务 */
    size_t current_idx = 0;
    for (int i = 0; i < g_thread_pool->num_threads && current_idx < rule_set->size; i++) {
        rule_match_task_t *task = calloc(1, sizeof(rule_match_task_t));
        if (!task) {
            continue;
        }
        
        task->event = event;
        task->fd = fd;
        task->rule_start = current_idx;
        task->rule_end = current_idx + rules_per_thread;
        
        /* 最后一个线程处理剩余的规则 */
        if (i == g_thread_pool->num_threads - 1) {
            task->rule_end += remaining_rules;
        }
        
        task->match_result = &match_result;
        task->result_mutex = &result_mutex;
        
        /* 提交任务 */
        if (submit_task_to_pool(task) != 0) {
            free(task);
        }
        
        current_idx = task->rule_end;
    }
    
    /* 等待所有任务完成 */
    /* 注意：这里使用了简单的轮询等待，实际应用中可能需要更优雅的等待机制 */
    while (1) {
        pthread_mutex_lock(&g_thread_pool->queue_mutex);
        int queue_count = g_thread_pool->queue_count;
        pthread_mutex_unlock(&g_thread_pool->queue_mutex);
        
        if (queue_count == 0) {
            break;
        }
        
        usleep(1000); /* 等待1毫秒 */
    }
    
    pthread_mutex_destroy(&result_mutex);
    
    return match_result;
}