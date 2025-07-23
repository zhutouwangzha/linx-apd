#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "linx_event_processor.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_rule_engine_set.h"
#include "linx_alert.h"

/* 全局标志用于优雅退出 */
static volatile bool g_running = true;
static linx_event_processor_t *g_processor = NULL;

/* 信号处理函数 */
void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nReceived signal %d, shutting down...\n", sig);
        g_running = false;
    }
}

/* 打印统计信息 */
void print_stats(void)
{
    linx_event_processor_stats_t stats;
    if (linx_event_processor_get_stats(g_processor, &stats) != 0) {
        return;
    }
    
    printf("\n=== Event Processor Statistics ===\n");
    printf("State: %s\n", (stats.total_events_received > 0) ? "Active" : "Idle");
    printf("Total Events: %lu\n", stats.total_events_received);
    printf("Processed Events: %lu\n", stats.total_events_processed);
    printf("Matched Events: %lu\n", stats.total_events_matched);
    printf("Failed Events: %lu\n", stats.total_events_failed);
    printf("Events/sec: %lu\n", stats.events_per_second);
    printf("Active Fetcher Threads: %u\n", stats.fetcher_active_threads);
    printf("Active Matcher Threads: %u\n", stats.matcher_active_threads);
    printf("Queue Size: %u\n", stats.queue_current_size);
    printf("================================\n\n");
}

/* 简单的统计回调 */
void stats_callback(const void *stats_ptr)
{
    const linx_event_processor_stats_t *stats = 
        (const linx_event_processor_stats_t *)stats_ptr;
    
    static time_t last_print = 0;
    time_t now = time(NULL);
    
    /* 每30秒打印一次简要统计 */
    if (now - last_print >= 30) {
        printf("[%ld] Events: %lu, Processed: %lu, Rate: %lu/s\n", 
               now, stats->total_events_received, 
               stats->total_events_processed, stats->events_per_second);
        last_print = now;
    }
}

/* 错误回调 */
void error_callback(const char *error_msg)
{
    printf("ERROR: %s\n", error_msg);
}

int main(int argc, char *argv[])
{
    linx_event_processor_config_t config;
    linx_ebpf_t ebpf_manager;
    int ret;
    time_t last_stats_time = 0;
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("LINX Event Processor Simple Example\n");
    printf("===================================\n");
    
    /* 初始化eBPF管理器 */
    ret = linx_ebpf_init(&ebpf_manager);
    if (ret != 0) {
        printf("Failed to initialize eBPF manager: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("✓ eBPF manager initialized\n");
    
    /* 初始化规则引擎 */
    ret = linx_rule_set_init();
    if (ret != 0) {
        printf("Failed to initialize rule set: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("✓ Rule engine initialized\n");
    
    /* 初始化告警系统 */
    ret = linx_alert_init(4);  /* 使用4个告警线程 */
    if (ret != 0) {
        printf("Failed to initialize alert system: %d\n", ret);
        linx_rule_set_deinit();
        return EXIT_FAILURE;
    }
    printf("✓ Alert system initialized\n");
    
    /* 获取默认配置并进行简单定制 */
    linx_event_processor_get_default_config(&config);
    
    /* 设置较小的线程数用于演示 */
    config.fetcher_thread_count = 2;    /* 2个获取线程 */
    config.matcher_thread_count = 4;    /* 4个匹配线程 */
    config.queue_config.capacity = 1000; /* 较小的队列 */
    
    /* 启用监控和回调 */
    config.monitor_config.enable_metrics = true;
    config.monitor_config.stats_interval_ms = 5000;  /* 5秒统计间隔 */
    config.stats_callback = stats_callback;
    config.error_callback = error_callback;
    
    /* 创建事件处理器 */
    g_processor = linx_event_processor_create(&config);
    if (!g_processor) {
        printf("Failed to create event processor\n");
        goto cleanup;
    }
    printf("✓ Event processor created\n");
    
    printf("\nConfiguration:\n");
    printf("  Fetcher threads: %u\n", config.fetcher_thread_count);
    printf("  Matcher threads: %u\n", config.matcher_thread_count);
    printf("  Queue capacity: %u\n", config.queue_config.capacity);
    
    /* 启动事件处理器 */
    ret = linx_event_processor_start(g_processor, &ebpf_manager);
    if (ret != 0) {
        printf("Failed to start event processor: %d\n", ret);
        const char *error = linx_event_processor_get_last_error(g_processor);
        printf("Error details: %s\n", error);
        goto cleanup;
    }
    printf("✓ Event processor started\n");
    
    printf("\nEvent processor is running...\n");
    printf("Press Ctrl+C to stop\n\n");
    
    /* 主循环 */
    while (g_running) {
        sleep(5);
        
        /* 每30秒打印详细统计 */
        time_t current_time = time(NULL);
        if (current_time - last_stats_time >= 30) {
            print_stats();
            last_stats_time = current_time;
        }
        
        /* 检查处理器状态 */
        linx_event_processor_state_t state = linx_event_processor_get_state(g_processor);
        if (state == LINX_EVENT_PROC_ERROR) {
            printf("Event processor encountered an error, stopping...\n");
            const char *error = linx_event_processor_get_last_error(g_processor);
            printf("Error details: %s\n", error);
            break;
        }
    }
    
    printf("\nShutting down event processor...\n");
    
    /* 停止事件处理器 */
    ret = linx_event_processor_stop(g_processor, 5000);  /* 5秒超时 */
    if (ret != 0) {
        printf("Warning: Failed to stop event processor gracefully: %d\n", ret);
    } else {
        printf("✓ Event processor stopped\n");
    }
    
    /* 打印最终统计信息 */
    print_stats();
    
cleanup:
    /* 清理资源 */
    if (g_processor) {
        linx_event_processor_destroy(g_processor);
        printf("✓ Event processor destroyed\n");
    }
    
    linx_alert_deinit();
    printf("✓ Alert system cleaned up\n");
    
    linx_rule_set_deinit();
    printf("✓ Rule engine cleaned up\n");
    
    printf("Simple example completed\n");
    return EXIT_SUCCESS;
}