#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <sys/resource.h>
#include <signal.h>

#include "linx_process_cache_ebpf.h"
#include "linx_process_cache.h"

/* eBPF程序和Map的文件描述符 */
static int prog_fd = -1;
static int map_fd = -1;
static int perf_fd = -1;
static struct perf_buffer *pb = NULL;

/* 事件处理器 */
static process_event_handler_t g_event_handler = NULL;

/* 消费者线程 */
static pthread_t consumer_thread;
static volatile int consumer_running = 0;

/* eBPF程序字节码 - 这里需要编译eBPF程序后的字节码 */
extern const unsigned char process_monitor_bpf_prog[];
extern const size_t process_monitor_bpf_prog_len;

/**
 * @brief 处理来自eBPF的进程事件
 */
static void handle_event(void *ctx, int cpu, void *data, unsigned int data_sz)
{
    (void)ctx;
    (void)cpu;
    
    if (data_sz != sizeof(struct process_event)) {
        fprintf(stderr, "Invalid event size: %u\n", data_sz);
        return;
    }
    
    struct process_event *event = (struct process_event *)data;
    
    /* 调用注册的事件处理器 */
    if (g_event_handler) {
        g_event_handler(event);
    }
    
    /* 默认处理：更新进程缓存 */
    switch (event->event_type) {
        case PROCESS_EVENT_FORK:
        case PROCESS_EVENT_EXEC:
            /* 进程创建或执行，添加到缓存 */
            linx_process_cache_update(event->pid);
            break;
            
        case PROCESS_EVENT_EXIT:
            /* 进程退出，标记为已退出但不立即删除 */
            {
                process_info_t *info = linx_process_cache_get(event->pid);
                if (info) {
                    info->is_alive = 0;
                    info->exit_time = event->exit_time / 1000000000; /* 纳秒转秒 */
                    info->state = PROCESS_STATE_EXITED;
                }
            }
            break;
            
        case PROCESS_EVENT_COMM_CHANGE:
            /* 进程名变化，更新缓存 */
            linx_process_cache_update(event->pid);
            break;
            
        default:
            break;
    }
}

/**
 * @brief 处理perf buffer丢失的事件
 */
static void handle_lost_events(void *ctx, int cpu, unsigned long long lost_cnt)
{
    (void)ctx;
    fprintf(stderr, "Lost %llu events on CPU #%d\n", lost_cnt, cpu);
}

/**
 * @brief 消费者线程函数
 */
static void *consumer_thread_func(void *arg)
{
    (void)arg;
    
    while (consumer_running) {
        /* 轮询perf buffer中的事件 */
        int ret = perf_buffer__poll(pb, 100); /* 100ms超时 */
        if (ret < 0 && ret != -EINTR) {
            fprintf(stderr, "Error polling perf buffer: %d\n", ret);
            break;
        }
    }
    
    return NULL;
}

/**
 * @brief 初始化eBPF进程监控
 */
int linx_process_ebpf_init(void)
{
    struct rlimit rlim = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };
    
    /* 设置内存限制 */
    if (setrlimit(RLIMIT_MEMLOCK, &rlim)) {
        fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK limit!\n");
        return -1;
    }
    
    /* 加载eBPF程序 */
    prog_fd = bpf_load_program(BPF_PROG_TYPE_TRACEPOINT,
                               (const struct bpf_insn *)process_monitor_bpf_prog,
                               process_monitor_bpf_prog_len / sizeof(struct bpf_insn),
                               "GPL", 0, NULL, 0);
    if (prog_fd < 0) {
        fprintf(stderr, "Failed to load eBPF program: %s\n", strerror(errno));
        return -1;
    }
    
    /* 创建perf event map */
    map_fd = bpf_create_map(BPF_MAP_TYPE_PERF_EVENT_ARRAY, sizeof(int), sizeof(int), 128, 0);
    if (map_fd < 0) {
        fprintf(stderr, "Failed to create perf event map: %s\n", strerror(errno));
        close(prog_fd);
        return -1;
    }
    
    /* 创建perf buffer */
    pb = perf_buffer__new(map_fd, 64, handle_event, handle_lost_events, NULL, NULL);
    if (!pb) {
        fprintf(stderr, "Failed to create perf buffer\n");
        close(map_fd);
        close(prog_fd);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 销毁eBPF进程监控
 */
void linx_process_ebpf_destroy(void)
{
    if (consumer_running) {
        linx_process_ebpf_stop_consumer();
    }
    
    if (pb) {
        perf_buffer__free(pb);
        pb = NULL;
    }
    
    if (map_fd >= 0) {
        close(map_fd);
        map_fd = -1;
    }
    
    if (prog_fd >= 0) {
        close(prog_fd);
        prog_fd = -1;
    }
}

/**
 * @brief 启动eBPF事件消费者线程
 */
int linx_process_ebpf_start_consumer(void)
{
    if (consumer_running) {
        return 0; /* 已经在运行 */
    }
    
    consumer_running = 1;
    
    if (pthread_create(&consumer_thread, NULL, consumer_thread_func, NULL) != 0) {
        consumer_running = 0;
        return -1;
    }
    
    return 0;
}

/**
 * @brief 停止eBPF事件消费者线程
 */
void linx_process_ebpf_stop_consumer(void)
{
    if (!consumer_running) {
        return;
    }
    
    consumer_running = 0;
    pthread_join(consumer_thread, NULL);
}

/**
 * @brief 注册事件处理器
 */
int linx_process_ebpf_register_handler(process_event_handler_t handler)
{
    g_event_handler = handler;
    return 0;
}