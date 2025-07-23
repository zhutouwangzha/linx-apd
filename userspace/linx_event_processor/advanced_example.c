#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/resource.h>

#include "linx_event_processor.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_rule_engine_set.h"
#include "linx_alert.h"

/* 全局变量 */
static volatile bool g_running = true;
static linx_event_processor_t *g_processor = NULL;

/* 统计回调函数 */
void stats_callback(const void *stats_ptr)
{
    const linx_event_processor_stats_t *stats = 
        (const linx_event_processor_stats_t *)stats_ptr;
    
    static uint64_t last_print_time = 0;
    uint64_t current_time = time(NULL);
    
    /* 每10秒打印一次详细统计 */
    if (current_time - last_print_time >= 10) {
        printf("\n=================== Event Processor Statistics ===================\n");
        printf("Runtime: %lu seconds\n", stats->uptime_seconds);
        printf("\nEvent Counts:\n");
        printf("  Received:    %12lu  Failed:      %12lu\n", 
               stats->total_events_received, stats->total_events_failed);
        printf("  Processed:   %12lu  Dropped:     %12lu\n", 
               stats->total_events_processed, stats->total_events_dropped);
        printf("  Matched:     %12lu\n", stats->total_events_matched);
        
        printf("\nThroughput:\n");
        printf("  Current:     %12lu events/sec\n", stats->events_per_second);
        printf("  Peak:        %12lu events/sec\n", stats->peak_events_per_second);
        printf("  Avg Latency: %12.2f μs\n", stats->avg_processing_latency_us);
        printf("  Max Latency: %12.2f μs\n", stats->max_processing_latency_us);
        
        printf("\nThread Pool Status:\n");
        printf("  Fetcher - Active: %3u  Idle: %3u\n", 
               stats->fetcher_active_threads, stats->fetcher_idle_threads);
        printf("  Matcher - Active: %3u  Idle: %3u\n", 
               stats->matcher_active_threads, stats->matcher_idle_threads);
        
        printf("\nQueue Status:\n");
        printf("  Current Size: %6u  Peak Size: %6u\n", 
               stats->queue_current_size, stats->queue_peak_size);
        printf("  Avg Wait:     %8.2f μs\n", stats->queue_avg_wait_time_us);
        
        printf("\nResource Usage:\n");
        printf("  Memory:       %12lu bytes\n", stats->memory_usage_bytes);
        printf("  CPU Usage:    %11.2f%%\n", stats->cpu_usage_percent);
        printf("  Context Sw:   %12lu\n", stats->context_switches);
        
        printf("\nError Counts:\n");
        printf("  Alloc Fail:   %12lu  Thread Fail: %12lu\n", 
               stats->allocation_failures, stats->thread_creation_failures);
        printf("  Queue Full:   %12lu\n", stats->queue_full_events);
        printf("==================================================================\n\n");
        
        last_print_time = current_time;
    }
}

/* 错误回调函数 */
void error_callback(const char *error_msg)
{
    printf("[ERROR] Event Processor: %s\n", error_msg);
}

/* 信号处理函数 */
void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nReceived signal %d, shutting down gracefully...\n", sig);
        g_running = false;
    } else if (sig == SIGUSR1) {
        /* 触发统计信息打印 */
        if (g_processor) {
            linx_event_processor_stats_t stats;
            if (linx_event_processor_get_stats(g_processor, &stats) == 0) {
                stats_callback(&stats);
            }
        }
    } else if (sig == SIGUSR2) {
        /* 重置统计信息 */
        if (g_processor) {
            linx_event_processor_reset_stats(g_processor);
            printf("Statistics reset.\n");
        }
    }
}

/* 命令行选项解析 */
struct program_options {
    uint32_t fetcher_threads;
    uint32_t matcher_threads;
    uint32_t queue_size;
    uint32_t batch_size;
    uint32_t memory_pool_size;
    bool enable_cpu_affinity;
    bool enable_profiling;
    bool enable_zero_copy;
    bool drop_on_full;
    int log_level;
    char *config_file;
};

void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -f, --fetcher-threads NUM    Number of fetcher threads (default: CPU count)\n");
    printf("  -m, --matcher-threads NUM    Number of matcher threads (default: CPU count * 2)\n");
    printf("  -q, --queue-size NUM         Event queue size (default: 10000)\n");
    printf("  -b, --batch-size NUM         Batch processing size (default: 32)\n");
    printf("  -p, --memory-pool-size NUM   Memory pool size (default: 1000)\n");
    printf("  -a, --[no-]cpu-affinity      Enable/disable CPU affinity (default: enabled)\n");
    printf("  -P, --[no-]profiling         Enable/disable profiling (default: disabled)\n");
    printf("  -z, --[no-]zero-copy         Enable/disable zero-copy (default: disabled)\n");
    printf("  -d, --[no-]drop-on-full      Drop events when queue is full (default: block)\n");
    printf("  -l, --log-level LEVEL        Log level (0-4, default: 2)\n");
    printf("  -c, --config-file FILE       Configuration file path\n");
    printf("  -h, --help                   Show this help message\n");
    printf("\nSignals:\n");
    printf("  SIGUSR1                      Print current statistics\n");
    printf("  SIGUSR2                      Reset statistics\n");
    printf("  SIGINT/SIGTERM               Graceful shutdown\n");
    printf("\nExamples:\n");
    printf("  %s --fetcher-threads 8 --matcher-threads 16\n", program_name);
    printf("  %s --queue-size 50000 --no-cpu-affinity\n", program_name);
    printf("  %s --profiling --log-level 3\n", program_name);
}

int parse_options(int argc, char *argv[], struct program_options *opts)
{
    static struct option long_options[] = {
        {"fetcher-threads",   required_argument, 0, 'f'},
        {"matcher-threads",   required_argument, 0, 'm'},
        {"queue-size",        required_argument, 0, 'q'},
        {"batch-size",        required_argument, 0, 'b'},
        {"memory-pool-size",  required_argument, 0, 'p'},
        {"cpu-affinity",      no_argument,       0, 'a'},
        {"no-cpu-affinity",   no_argument,       0, 'A'},
        {"profiling",         no_argument,       0, 'P'},
        {"no-profiling",      no_argument,       0, 'Q'},
        {"zero-copy",         no_argument,       0, 'z'},
        {"no-zero-copy",      no_argument,       0, 'Z'},
        {"drop-on-full",      no_argument,       0, 'd'},
        {"no-drop-on-full",   no_argument,       0, 'D'},
        {"log-level",         required_argument, 0, 'l'},
        {"config-file",       required_argument, 0, 'c'},
        {"help",              no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    /* 设置默认值 */
    opts->fetcher_threads = 0;     /* 0表示使用默认值 */
    opts->matcher_threads = 0;
    opts->queue_size = 10000;
    opts->batch_size = 32;
    opts->memory_pool_size = 1000;
    opts->enable_cpu_affinity = true;
    opts->enable_profiling = false;
    opts->enable_zero_copy = false;
    opts->drop_on_full = false;
    opts->log_level = 2;
    opts->config_file = NULL;
    
    int c;
    while ((c = getopt_long(argc, argv, "f:m:q:b:p:aAPQzZdDl:c:h", 
                           long_options, NULL)) != -1) {
        switch (c) {
        case 'f':
            opts->fetcher_threads = (uint32_t)atoi(optarg);
            break;
        case 'm':
            opts->matcher_threads = (uint32_t)atoi(optarg);
            break;
        case 'q':
            opts->queue_size = (uint32_t)atoi(optarg);
            break;
        case 'b':
            opts->batch_size = (uint32_t)atoi(optarg);
            break;
        case 'p':
            opts->memory_pool_size = (uint32_t)atoi(optarg);
            break;
        case 'a':
            opts->enable_cpu_affinity = true;
            break;
        case 'A':
            opts->enable_cpu_affinity = false;
            break;
        case 'P':
            opts->enable_profiling = true;
            break;
        case 'Q':
            opts->enable_profiling = false;
            break;
        case 'z':
            opts->enable_zero_copy = true;
            break;
        case 'Z':
            opts->enable_zero_copy = false;
            break;
        case 'd':
            opts->drop_on_full = true;
            break;
        case 'D':
            opts->drop_on_full = false;
            break;
        case 'l':
            opts->log_level = atoi(optarg);
            break;
        case 'c':
            opts->config_file = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 1;
        default:
            fprintf(stderr, "Invalid option. Use -h for help.\n");
            return -1;
        }
    }
    
    return 0;
}

/* 创建配置 */
void create_config(const struct program_options *opts, 
                  linx_event_processor_config_t *config)
{
    /* 获取默认配置 */
    linx_event_processor_get_default_config(config);
    
    /* 应用命令行选项 */
    if (opts->fetcher_threads > 0) {
        config->fetcher_thread_count = opts->fetcher_threads;
        config->fetcher_pool_config.min_threads = opts->fetcher_threads;
        config->fetcher_pool_config.max_threads = opts->fetcher_threads;
    }
    
    if (opts->matcher_threads > 0) {
        config->matcher_thread_count = opts->matcher_threads;
        config->matcher_pool_config.min_threads = opts->matcher_threads;
        config->matcher_pool_config.max_threads = opts->matcher_threads;
    }
    
    config->queue_config.capacity = opts->queue_size;
    config->queue_config.batch_size = opts->batch_size;
    config->queue_config.drop_on_full = opts->drop_on_full;
    config->perf_config.memory_pool_size = opts->memory_pool_size;
    
    config->fetcher_pool_config.cpu_affinity = opts->enable_cpu_affinity;
    config->matcher_pool_config.cpu_affinity = opts->enable_cpu_affinity;
    
    config->perf_config.enable_zero_copy = opts->enable_zero_copy;
    config->monitor_config.enable_cpu_profiling = opts->enable_profiling;
    
    /* 设置回调函数 */
    config->stats_callback = stats_callback;
    config->error_callback = error_callback;
}

/* 设置系统资源限制 */
void setup_resource_limits(void)
{
    struct rlimit rlim;
    
    /* 设置核心转储大小 */
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
    
    /* 设置文件描述符限制 */
    rlim.rlim_cur = 65536;
    rlim.rlim_max = 65536;
    setrlimit(RLIMIT_NOFILE, &rlim);
    
    /* 设置内存锁定限制 */
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_MEMLOCK, &rlim);
}

int main(int argc, char *argv[])
{
    struct program_options opts;
    linx_event_processor_config_t config;
    linx_ebpf_t ebpf_manager;
    int ret;
    
    /* 解析命令行选项 */
    ret = parse_options(argc, argv, &opts);
    if (ret != 0) {
        return ret > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    /* 设置系统资源限制 */
    setup_resource_limits();
    
    printf("LINX Advanced Event Processor Example\n");
    printf("=====================================\n");
    
    /* 初始化日志系统 */
    // ret = linx_log_init(opts.log_level);
    // if (ret != 0) {
    //     fprintf(stderr, "Failed to initialize logging: %d\n", ret);
    //     return EXIT_FAILURE;
    // }
    
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
        printf("Failed to initialize rule engine: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("✓ Rule engine initialized\n");
    
    /* 初始化告警系统 */
    ret = linx_alert_init(8);
    if (ret != 0) {
        printf("Failed to initialize alert system: %d\n", ret);
        linx_rule_set_deinit();
        return EXIT_FAILURE;
    }
    printf("✓ Alert system initialized\n");
    
    /* 创建配置 */
    create_config(&opts, &config);
    
    /* 验证配置 */
    ret = linx_event_processor_validate_config(&config);
    if (ret != 0) {
        printf("Invalid configuration: %d\n", ret);
        goto cleanup;
    }
    
    /* 创建事件处理器 */
    g_processor = linx_event_processor_create(&config);
    if (!g_processor) {
        printf("Failed to create event processor\n");
        goto cleanup;
    }
    printf("✓ Event processor created\n");
    
    /* 打印配置信息 */
    printf("\nConfiguration:\n");
    printf("  Fetcher threads:   %u\n", config.fetcher_thread_count);
    printf("  Matcher threads:   %u\n", config.matcher_thread_count);
    printf("  Queue capacity:    %u\n", config.queue_config.capacity);
    printf("  Batch size:        %u\n", config.queue_config.batch_size);
    printf("  Memory pool size:  %u\n", config.perf_config.memory_pool_size);
    printf("  CPU affinity:      %s\n", config.fetcher_pool_config.cpu_affinity ? "enabled" : "disabled");
    printf("  Zero-copy:         %s\n", config.perf_config.enable_zero_copy ? "enabled" : "disabled");
    printf("  Drop on full:      %s\n", config.queue_config.drop_on_full ? "enabled" : "disabled");
    printf("  Profiling:         %s\n", config.monitor_config.enable_cpu_profiling ? "enabled" : "disabled");
    
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
    printf("Send SIGUSR1 (kill -USR1 %d) to print statistics\n", getpid());
    printf("Send SIGUSR2 (kill -USR2 %d) to reset statistics\n", getpid());
    printf("Press Ctrl+C to stop\n\n");
    
    /* 主循环 */
    time_t start_time = time(NULL);
    while (g_running) {
        sleep(1);
        
        /* 检查处理器状态 */
        linx_event_processor_state_t state = linx_event_processor_get_state(g_processor);
        if (state == LINX_EVENT_PROC_ERROR) {
            printf("Event processor encountered an error, stopping...\n");
            const char *error = linx_event_processor_get_last_error(g_processor);
            printf("Error details: %s\n", error);
            break;
        }
        
        /* 每分钟打印简要状态 */
        time_t current_time = time(NULL);
        if ((current_time - start_time) % 60 == 0 && current_time != start_time) {
            linx_event_processor_stats_t stats;
            if (linx_event_processor_get_stats(g_processor, &stats) == 0) {
                printf("[%lu] Processed: %lu, Matched: %lu, Rate: %lu/s\n",
                       current_time - start_time,
                       stats.total_events_processed,
                       stats.total_events_matched,
                       stats.events_per_second);
            }
        }
    }
    
    printf("\nShutting down event processor...\n");
    
    /* 停止事件处理器 */
    ret = linx_event_processor_stop(g_processor, 10000);  /* 10秒超时 */
    if (ret != 0) {
        printf("Warning: Failed to stop event processor gracefully: %d\n", ret);
    } else {
        printf("✓ Event processor stopped\n");
    }
    
    /* 打印最终统计信息 */
    linx_event_processor_stats_t final_stats;
    if (linx_event_processor_get_stats(g_processor, &final_stats) == 0) {
        printf("\nFinal Statistics:\n");
        printf("  Total events processed: %lu\n", final_stats.total_events_processed);
        printf("  Total events matched:   %lu\n", final_stats.total_events_matched);
        printf("  Total events dropped:   %lu\n", final_stats.total_events_dropped);
        printf("  Peak throughput:        %lu events/sec\n", final_stats.peak_events_per_second);
        printf("  Runtime:                %lu seconds\n", (unsigned long)(time(NULL) - start_time));
    }
    
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
    
    printf("✓ All resources cleaned up\n");
    printf("Event processor example completed\n");
    
    return EXIT_SUCCESS;
}