#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "linx_event_rich.h"
#include "linx_thread_context.h"
#include "linx_thread_base_addr.h"
#include "linx_log.h"

#define MAX_WORKER_THREADS 8
#define MAX_EVENTS_PER_THREAD 5000

/* 全局停止标志 */
static volatile bool g_stop_flag = false;

/* 工作线程参数 */
typedef struct {
    int thread_id;
    int events_to_process;
    pthread_barrier_t *start_barrier;
} worker_args_t;

/* 创建模拟事件（仅用于演示） */
linx_event_t *create_mock_event(int event_type, int thread_id, int seq)
{
    linx_event_t *event = calloc(1, sizeof(linx_event_t) + 256);
    if (!event) return NULL;
    
    event->type = event_type % 20; /* 模拟不同的事件类型 */
    event->time = time(NULL) * 1000000000ULL + seq; /* 纳秒时间戳 */
    event->pid = getpid() + thread_id;
    event->tid = pthread_self();
    event->size = sizeof(linx_event_t) + 256;
    snprintf(event->comm, sizeof(event->comm), "thread_%d_proc", thread_id);
    snprintf(event->cmdline, sizeof(event->cmdline), "thread_%d_proc arg%d", thread_id, seq);
    
    return event;
}

/* 优化的工作线程函数 */
void *optimized_worker_thread(void *arg)
{
    worker_args_t *worker_args = (worker_args_t *)arg;
    int processed_count = 0;
    int failed_count = 0;
    clock_t start_time, end_time;
    
    LINX_LOG_INFO("Optimized worker thread %d started", worker_args->thread_id);
    
    /* 为当前线程创建上下文（只包含event_t，base_addr通过专门机制管理） */
    if (!linx_thread_context_create()) {
        LINX_LOG_ERROR("Failed to create thread context for worker %d", 
                      worker_args->thread_id);
        return NULL;
    }
    
    /* 等待所有线程准备就绪 */
    pthread_barrier_wait(worker_args->start_barrier);
    
    start_time = clock();
    
    /* 主工作循环 */
    for (int i = 0; i < worker_args->events_to_process && !g_stop_flag; i++) {
        /* 创建模拟事件 */
        linx_event_t *event = create_mock_event(i, worker_args->thread_id, i);
        if (!event) {
            failed_count++;
            continue;
        }
        
        /* 处理事件 */
        int ret = linx_event_rich(event);
        if (ret == 0) {
            /* 获取处理结果 */
            event_t *rich_event = linx_event_rich_get();
            if (rich_event) {
                processed_count++;
                
                /* 每处理1000个事件打印一次进度 */
                if (processed_count % 1000 == 0) {
                    LINX_LOG_INFO("Worker %d processed %d events", 
                                worker_args->thread_id, processed_count);
                }
            }
        } else {
            failed_count++;
        }
        
        /* 释放事件内存 */
        free(event);
    }
    
    end_time = clock();
    double cpu_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    /* 清理线程上下文 */
    linx_thread_context_destroy();
    
    LINX_LOG_INFO("Optimized worker thread %d completed: processed=%d, failed=%d, time=%.3fs, rate=%.0f events/sec", 
                  worker_args->thread_id, processed_count, failed_count, cpu_time,
                  cpu_time > 0 ? processed_count / cpu_time : 0);
    
    return NULL;
}

/* 信号处理函数 */
void signal_handler(int sig)
{
    LINX_LOG_INFO("Received signal %d, stopping...", sig);
    g_stop_flag = true;
}

/* 性能测试函数 */
void performance_test(int num_threads, int events_per_thread)
{
    pthread_t worker_threads[MAX_WORKER_THREADS];
    worker_args_t worker_args[MAX_WORKER_THREADS];
    pthread_barrier_t start_barrier;
    clock_t start_time, end_time;
    
    LINX_LOG_INFO("Starting performance test with %d threads, %d events per thread", 
                  num_threads, events_per_thread);
    
    /* 初始化同步屏障 */
    pthread_barrier_init(&start_barrier, NULL, num_threads + 1);
    
    /* 创建工作线程 */
    for (int i = 0; i < num_threads; i++) {
        worker_args[i].thread_id = i;
        worker_args[i].events_to_process = events_per_thread;
        worker_args[i].start_barrier = &start_barrier;
        
        int ret = pthread_create(&worker_threads[i], NULL, optimized_worker_thread, &worker_args[i]);
        if (ret) {
            LINX_LOG_ERROR("Failed to create worker thread %d: %s", i, strerror(ret));
            g_stop_flag = true;
            break;
        }
    }
    
    /* 等待所有线程准备就绪，然后开始计时 */
    pthread_barrier_wait(&start_barrier);
    start_time = clock();
    
    LINX_LOG_INFO("All threads started, processing events...");
    
    /* 等待工作线程结束 */
    for (int i = 0; i < num_threads; i++) {
        if (worker_threads[i]) {
            pthread_join(worker_threads[i], NULL);
        }
    }
    
    end_time = clock();
    double total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    int total_events = num_threads * events_per_thread;
    
    LINX_LOG_INFO("Performance test completed:");
    LINX_LOG_INFO("  Total threads: %d", num_threads);
    LINX_LOG_INFO("  Events per thread: %d", events_per_thread);
    LINX_LOG_INFO("  Total events: %d", total_events);
    LINX_LOG_INFO("  Total time: %.3f seconds", total_time);
    LINX_LOG_INFO("  Overall throughput: %.0f events/sec", 
                  total_time > 0 ? total_events / total_time : 0);
    
    pthread_barrier_destroy(&start_barrier);
}

int main(int argc, char *argv[])
{
    int num_threads = 4;
    int events_per_thread = 1000;
    
    /* 解析命令行参数 */
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads <= 0 || num_threads > MAX_WORKER_THREADS) {
            num_threads = 4;
        }
    }
    if (argc > 2) {
        events_per_thread = atoi(argv[2]);
        if (events_per_thread <= 0 || events_per_thread > MAX_EVENTS_PER_THREAD) {
            events_per_thread = 1000;
        }
    }
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    LINX_LOG_INFO("Starting optimized multi-thread event rich example");
    LINX_LOG_INFO("Configuration: %d threads, %d events per thread", num_threads, events_per_thread);
    
    /* 初始化事件丰富化系统（共享字段映射） */
    int ret = linx_event_rich_init();
    if (ret) {
        LINX_LOG_ERROR("Failed to initialize event rich system");
        return -1;
    }
    
    /* 运行性能测试 */
    performance_test(num_threads, events_per_thread);
    
    /* 清理资源 */
    linx_event_rich_deinit();
    
    LINX_LOG_INFO("Optimized multi-thread event rich example completed");
    return 0;
}