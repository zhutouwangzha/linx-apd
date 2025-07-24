#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "linx_process_cache.h"
#include "linx_process_cache_ebpf.h"

/* 全局控制变量 */
static volatile int g_running = 1;

/* 信号处理函数 */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* 自定义进程事件处理器 */
static int custom_process_event_handler(const struct process_event *event)
{
    switch (event->event_type) {
        case PROCESS_EVENT_FORK:
            printf("[FORK] PID: %u, PPID: %u, COMM: %s\n", 
                   event->pid, event->ppid, event->comm);
            break;
            
        case PROCESS_EVENT_EXEC:
            printf("[EXEC] PID: %u, PPID: %u, COMM: %s, FILE: %s\n", 
                   event->pid, event->ppid, event->comm, event->filename);
            break;
            
        case PROCESS_EVENT_EXIT:
            {
                unsigned long long runtime_ms = 0;
                if (event->exit_time > event->start_time) {
                    runtime_ms = (event->exit_time - event->start_time) / 1000000;
                }
                
                printf("[EXIT] PID: %u, COMM: %s, Runtime: %llu ms", 
                       event->pid, event->comm, runtime_ms);
                       
                if (runtime_ms < 200) {
                    printf(" [SHORT PROCESS]");
                }
                printf("\n");
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

/* 测试短进程的函数 */
static void test_short_processes(void)
{
    printf("\n=== 测试短进程检测 ===\n");
    
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            /* 子进程 - 执行短暂的任务后退出 */
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "short_test_%d", i);
            
            /* 模拟短暂的工作 */
            usleep(50000 + (i * 20000)); /* 50-130ms */
            
            exit(0);
        } else if (pid > 0) {
            printf("启动短进程 PID: %d\n", pid);
        } else {
            perror("fork failed");
        }
        
        usleep(100000); /* 等待100ms再启动下一个 */
    }
    
    /* 等待所有子进程结束 */
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        usleep(10000);
    }
}

/* 显示缓存统计信息 */
static void show_cache_stats(void)
{
    int total, alive, expired;
    linx_process_cache_stats(&total, &alive, &expired);
    
    printf("\n=== 进程缓存统计 ===\n");
    printf("总进程数: %d\n", total);
    printf("存活进程: %d\n", alive);
    printf("已退出进程: %d\n", expired);
}

int main(int argc, char *argv[])
{
    int use_ebpf = 1;
    int use_fast_scan = 0;
    
    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-ebpf") == 0) {
            use_ebpf = 0;
        } else if (strcmp(argv[i], "--fast-scan") == 0) {
            use_fast_scan = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("用法: %s [选项]\n", argv[0]);
            printf("选项:\n");
            printf("  --no-ebpf      禁用eBPF监控\n");
            printf("  --fast-scan    启用快速扫描模式\n");
            printf("  --help         显示此帮助信息\n");
            return 0;
        }
    }
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== 进程缓存增强版示例 ===\n");
    
    /* 初始化进程缓存 */
    if (linx_process_cache_init() != 0) {
        fprintf(stderr, "Failed to initialize process cache\n");
        return 1;
    }
    
    /* 尝试启用eBPF监控 */
    if (use_ebpf) {
        if (linx_process_ebpf_init() == 0) {
            printf("✓ eBPF进程监控已启用\n");
            
            /* 注册自定义事件处理器 */
            linx_process_ebpf_register_handler(custom_process_event_handler);
            
            /* 启动eBPF事件消费者 */
            if (linx_process_ebpf_start_consumer() == 0) {
                printf("✓ eBPF事件消费者已启动\n");
            } else {
                printf("✗ eBPF事件消费者启动失败\n");
                use_ebpf = 0;
            }
        } else {
            printf("✗ eBPF不可用，使用传统方法\n");
            use_ebpf = 0;
        }
    }
    
    /* 如果eBPF不可用且启用了快速扫描 */
    if (!use_ebpf && use_fast_scan) {
        extern int linx_process_cache_enable_fast_scan(void);
        if (linx_process_cache_enable_fast_scan() == 0) {
            printf("✓ 快速扫描模式已启用\n");
        } else {
            printf("✗ 快速扫描模式启动失败\n");
        }
    }
    
    /* 显示初始统计信息 */
    show_cache_stats();
    
    /* 测试短进程检测 */
    test_short_processes();
    
    printf("\n等待10秒以观察进程事件...\n");
    for (int i = 0; i < 10 && g_running; i++) {
        sleep(1);
        
        /* 每2秒显示一次统计信息 */
        if (i % 2 == 1) {
            show_cache_stats();
        }
    }
    
    /* 如果使用了增强功能，显示短进程记录 */
    if (use_fast_scan) {
        extern int linx_process_cache_get_short_processes(void **list, int *count);
        void *short_processes;
        int count;
        
        if (linx_process_cache_get_short_processes(&short_processes, &count) == 0 && count > 0) {
            printf("\n=== 检测到的短进程 ===\n");
            printf("共检测到 %d 个短进程\n", count);
            free(short_processes);
        }
    }
    
    /* 最终统计信息 */
    show_cache_stats();
    
    /* 清理资源 */
    if (use_ebpf) {
        linx_process_ebpf_destroy();
        printf("eBPF监控已停止\n");
    }
    
    if (use_fast_scan) {
        extern void linx_process_cache_disable_fast_scan(void);
        linx_process_cache_disable_fast_scan();
        printf("快速扫描已停止\n");
    }
    
    linx_process_cache_destroy();
    printf("进程缓存已销毁\n");
    
    return 0;
}