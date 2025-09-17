#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "linx_event_rich_mt.h"
#include "linx_log.h"

#define MAX_WORKER_THREADS 4
#define MAX_EVENTS_PER_THREAD 1000

/* 全局停止标志 */
static volatile bool g_stop_flag = false;

/* 简单的事件队列结构（示例用） */
typedef struct event_queue {
    linx_event_t **events;
    int capacity;
    int size;
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} event_queue_t;

/* 创建事件队列 */
event_queue_t *create_event_queue(int capacity)
{
    event_queue_t *queue = calloc(1, sizeof(event_queue_t));
    if (!queue) return NULL;
    
    queue->events = calloc(capacity, sizeof(linx_event_t *));
    if (!queue->events) {
        free(queue);
        return NULL;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    
    return queue;
}

/* 销毁事件队列 */
void destroy_event_queue(event_queue_t *queue)
{
    if (!queue) return;
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    
    free(queue->events);
    free(queue);
}

/* 向队列添加事件 */
int enqueue_event(event_queue_t *queue, linx_event_t *event)
{
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == queue->capacity && !g_stop_flag) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (g_stop_flag) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    queue->events[queue->tail] = event;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

/* 从队列获取事件 */
linx_event_t *dequeue_event(event_queue_t *queue)
{
    linx_event_t *event = NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == 0 && !g_stop_flag) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (g_stop_flag && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    if (queue->size > 0) {
        event = queue->events[queue->head];
        queue->head = (queue->head + 1) % queue->capacity;
        queue->size--;
        
        pthread_cond_signal(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return event;
}

/* 自定义的工作线程函数 */
void *custom_worker_thread(void *arg)
{
    linx_worker_args_t *worker_args = (linx_worker_args_t *)arg;
    event_queue_t *queue = (event_queue_t *)worker_args->event_queue;
    int processed_count = 0;
    int ret;
    
    LINX_LOG_INFO("Custom worker thread %d started", worker_args->thread_id);
    
    /* 初始化当前线程的事件丰富化上下文 */
    ret = linx_event_rich_thread_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize thread context for worker %d", 
                      worker_args->thread_id);
        return NULL;
    }
    
    /* 主工作循环 */
    while (!g_stop_flag) {
        /* 从队列获取事件 */
        linx_event_t *event = dequeue_event(queue);
        
        if (event) {
            /* 处理事件 */
            ret = linx_event_rich_mt(event);
            if (ret == 0) {
                /* 获取处理结果 */
                event_t *rich_event = linx_event_rich_mt_get();
                if (rich_event) {
                    processed_count++;
                    
                    /* 打印处理结果（示例） */
                    if (processed_count % 100 == 0) {
                        LINX_LOG_INFO("Worker %d processed %d events, latest: %s", 
                                    worker_args->thread_id, processed_count, 
                                    rich_event->type ? rich_event->type : "unknown");
                    }
                }
            } else {
                LINX_LOG_WARNING("Failed to process event in worker %d", 
                               worker_args->thread_id);
            }
            
            /* 释放事件内存（示例中假设事件需要释放） */
            free(event);
        }
    }
    
    /* 清理线程上下文 */
    linx_event_rich_thread_deinit();
    
    LINX_LOG_INFO("Custom worker thread %d stopped, processed %d events", 
                  worker_args->thread_id, processed_count);
    return NULL;
}

/* 信号处理函数 */
void signal_handler(int sig)
{
    LINX_LOG_INFO("Received signal %d, stopping...", sig);
    g_stop_flag = true;
}

/* 创建模拟事件（仅用于演示） */
linx_event_t *create_mock_event(int event_type)
{
    linx_event_t *event = calloc(1, sizeof(linx_event_t) + 256);
    if (!event) return NULL;
    
    event->type = event_type;
    event->time = time(NULL) * 1000000000ULL; /* 纳秒时间戳 */
    event->pid = getpid();
    event->tid = pthread_self();
    event->size = sizeof(linx_event_t) + 256;
    strcpy(event->comm, "test_process");
    strcpy(event->cmdline, "test_process arg1 arg2");
    
    return event;
}

int main(int argc, char *argv[])
{
    pthread_t worker_threads[MAX_WORKER_THREADS];
    linx_worker_args_t worker_args[MAX_WORKER_THREADS];
    event_queue_t *event_queue;
    int ret;
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    LINX_LOG_INFO("Starting multi-thread event rich example");
    
    /* 初始化多线程事件丰富化系统 */
    ret = linx_event_rich_mt_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize multi-thread system");
        return -1;
    }
    
    /* 创建事件队列 */
    event_queue = create_event_queue(1000);
    if (!event_queue) {
        LINX_LOG_ERROR("Failed to create event queue");
        linx_event_rich_mt_deinit();
        return -1;
    }
    
    /* 创建工作线程 */
    for (int i = 0; i < MAX_WORKER_THREADS; i++) {
        worker_args[i].thread_id = i;
        worker_args[i].event_queue = event_queue;
        worker_args[i].user_data = NULL;
        worker_args[i].stop_flag = &g_stop_flag;
        
        ret = pthread_create(&worker_threads[i], NULL, custom_worker_thread, &worker_args[i]);
        if (ret) {
            LINX_LOG_ERROR("Failed to create worker thread %d: %s", i, strerror(ret));
            g_stop_flag = true;
            break;
        }
    }
    
    /* 模拟事件生产者 */
    int event_count = 0;
    while (!g_stop_flag && event_count < MAX_EVENTS_PER_THREAD * MAX_WORKER_THREADS) {
        linx_event_t *event = create_mock_event(event_count % 10);
        if (event) {
            if (enqueue_event(event_queue, event) == 0) {
                event_count++;
            } else {
                free(event);
                break;
            }
        }
        
        /* 控制事件生产速度 */
        usleep(1000); /* 1ms */
    }
    
    LINX_LOG_INFO("Produced %d events, waiting for processing to complete...", event_count);
    
    /* 等待一段时间让工作线程处理完剩余事件 */
    sleep(2);
    
    /* 停止所有线程 */
    g_stop_flag = true;
    
    /* 唤醒所有等待的线程 */
    pthread_mutex_lock(&event_queue->mutex);
    pthread_cond_broadcast(&event_queue->not_empty);
    pthread_cond_broadcast(&event_queue->not_full);
    pthread_mutex_unlock(&event_queue->mutex);
    
    /* 等待工作线程结束 */
    for (int i = 0; i < MAX_WORKER_THREADS; i++) {
        if (worker_threads[i]) {
            pthread_join(worker_threads[i], NULL);
        }
    }
    
    /* 清理资源 */
    destroy_event_queue(event_queue);
    linx_event_rich_mt_deinit();
    
    LINX_LOG_INFO("Multi-thread event rich example completed");
    return 0;
}