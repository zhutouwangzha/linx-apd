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
    linx_event_processor_get_stats(&stats);
    
    printf("\n=== Event Processor Statistics ===\n");
    printf("Running: %s\n", stats.running ? "Yes" : "No");
    printf("Total Events: %lu\n", stats.total_events);
    printf("Processed Events: %lu\n", stats.processed_events);
    printf("Matched Events: %lu\n", stats.matched_events);
    printf("Failed Events: %lu\n", stats.failed_events);
    printf("Fetch Tasks Submitted: %lu\n", stats.fetch_tasks_submitted);
    printf("Match Tasks Submitted: %lu\n", stats.match_tasks_submitted);
    printf("Active Fetcher Threads: %d\n", stats.fetcher_active_threads);
    printf("Active Matcher Threads: %d\n", stats.matcher_active_threads);
    printf("Fetcher Queue Size: %d\n", stats.fetcher_queue_size);
    printf("Matcher Queue Size: %d\n", stats.matcher_queue_size);
    printf("================================\n\n");
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
    
    printf("Starting LINX Event Processor Example\n");
    
    /* 初始化日志系统（假设已有实现） */
    // linx_log_init();
    
    /* 初始化eBPF管理器 */
    ret = linx_ebpf_init(&ebpf_manager);
    if (ret != 0) {
        printf("Failed to initialize eBPF manager: %d\n", ret);
        return EXIT_FAILURE;
    }
    
    /* 初始化规则引擎 */
    ret = linx_rule_set_init();
    if (ret != 0) {
        printf("Failed to initialize rule set: %d\n", ret);
        return EXIT_FAILURE;
    }
    
    /* 初始化告警系统 */
    ret = linx_alert_init(4);  /* 使用4个告警线程 */
    if (ret != 0) {
        printf("Failed to initialize alert system: %d\n", ret);
        linx_rule_set_deinit();
        return EXIT_FAILURE;
    }
    
    /* 配置事件处理器参数 */
    config.event_fetcher_pool_size = 4;    /* 4个获取线程 */
    config.event_matcher_pool_size = 8;    /* 8个匹配线程 */
    config.event_queue_size = 1000;        /* 队列大小1000 */
    config.batch_size = 10;                /* 批处理大小10 */
    
    /* 初始化事件处理器 */
    ret = linx_event_processor_init(&config);
    if (ret != 0) {
        printf("Failed to initialize event processor: %d\n", ret);
        linx_alert_deinit();
        linx_rule_set_deinit();
        return EXIT_FAILURE;
    }
    
    printf("Event processor initialized successfully\n");
    printf("Configuration:\n");
    printf("  - Fetcher threads: %d\n", config.event_fetcher_pool_size);
    printf("  - Matcher threads: %d\n", config.event_matcher_pool_size);
    printf("  - Queue size: %d\n", config.event_queue_size);
    printf("  - Batch size: %d\n", config.batch_size);
    
    /* 启动事件处理器 */
    ret = linx_event_processor_start(&ebpf_manager);
    if (ret != 0) {
        printf("Failed to start event processor: %d\n", ret);
        linx_event_processor_deinit();
        linx_alert_deinit();
        linx_rule_set_deinit();
        return EXIT_FAILURE;
    }
    
    printf("Event processor started, processing events...\n");
    printf("Press Ctrl+C to stop\n");
    
    /* 主循环 - 定期打印统计信息 */
    while (g_running) {
        sleep(1);
        
        time_t current_time = time(NULL);
        if (current_time - last_stats_time >= 10) {  /* 每10秒打印一次统计 */
            print_stats();
            last_stats_time = current_time;
        }
        
        /* 检查处理器是否还在运行 */
        if (!linx_event_processor_is_running()) {
            printf("Event processor stopped unexpectedly\n");
            break;
        }
    }
    
    printf("Shutting down event processor...\n");
    
    /* 停止事件处理器 */
    ret = linx_event_processor_stop();
    if (ret != 0) {
        printf("Warning: Failed to stop event processor gracefully: %d\n", ret);
    }
    
    /* 打印最终统计信息 */
    print_stats();
    
    /* 清理资源 */
    linx_event_processor_deinit();
    linx_alert_deinit();
    linx_rule_set_deinit();
    
    printf("Event processor example completed\n");
    return EXIT_SUCCESS;
}