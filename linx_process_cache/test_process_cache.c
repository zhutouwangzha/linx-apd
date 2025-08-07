#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "linx_process_cache.h"

static int g_running = 1;

void signal_handler(int sig)
{
    g_running = 0;
}

/**
 * @brief 打印进程信息
 */
void print_process_info(process_info_t *info)
{
    const char *state_str[] = {
        "Running", "Sleeping", "Zombie", "Stopped", "Exited", "Unknown"
    };
    
    printf("========================================\n");
    printf("PID: %d%s\n", info->pid, info->is_alive ? "" : " (EXITED)");
    printf("PPID: %d, PGID: %d, SID: %d\n", info->ppid, info->pgid, info->sid);
    printf("UID: %d, GID: %d\n", info->uid, info->gid);
    printf("State: %s\n", state_str[info->state]);
    printf("Command: %s\n", info->comm);
    printf("Cmdline: %s\n", info->cmdline[0] ? info->cmdline : "(none)");
    printf("Exe: %s\n", info->exe_path[0] ? info->exe_path : "(none)");
    printf("CWD: %s\n", info->cwd[0] ? info->cwd : "(none)");
    printf("Memory: VSize=%lu KB, RSS=%lu KB, Shared=%lu KB\n",
           info->vsize / 1024, info->rss / 1024, info->shared / 1024);
    printf("CPU Time: User=%lu, System=%lu\n", info->utime, info->stime);
    printf("Nice: %d, Priority: %d\n", info->nice, info->priority);
    
    char time_buf[64];
    struct tm *tm_info = localtime(&info->update_time);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Last Update: %s\n", time_buf);
    
    if (!info->is_alive && info->exit_time > 0) {
        tm_info = localtime(&info->exit_time);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Exit Time: %s\n", time_buf);
    }
}

/**
 * @brief 测试基本功能
 */
void test_basic_functions()
{
    printf("\n=== Testing Basic Functions ===\n");
    
    /* 获取当前进程信息 */
    pid_t my_pid = getpid();
    process_info_t *info = linx_process_cache_get(my_pid);
    
    if (info) {
        printf("Current process info:\n");
        print_process_info(info);
    } else {
        printf("Failed to get current process info\n");
    }
    
    /* 获取父进程信息 */
    pid_t parent_pid = getppid();
    info = linx_process_cache_get(parent_pid);
    
    if (info) {
        printf("\nParent process info:\n");
        print_process_info(info);
    } else {
        printf("Failed to get parent process info\n");
    }
}

/**
 * @brief 测试子进程创建和退出
 */
void test_child_process()
{
    printf("\n=== Testing Child Process Creation and Exit ===\n");
    
    pid_t child_pid = fork();
    
    if (child_pid == 0) {
        /* 子进程 */
        printf("Child process created, PID=%d\n", getpid());
        sleep(5);
        printf("Child process exiting...\n");
        exit(0);
    } else if (child_pid > 0) {
        /* 父进程 */
        printf("Parent: created child PID=%d\n", child_pid);
        
        /* 等待子进程信息被缓存 */
        sleep(2);
        
        /* 获取子进程信息 */
        process_info_t *info = linx_process_cache_get(child_pid);
        if (info) {
            printf("Child process info while running:\n");
            print_process_info(info);
        }
        
        /* 等待子进程退出 */
        int status;
        waitpid(child_pid, &status, 0);
        printf("Parent: child process exited\n");
        
        /* 等待缓存更新 */
        sleep(2);
        
        /* 再次获取子进程信息 */
        info = linx_process_cache_get(child_pid);
        if (info) {
            printf("\nChild process info after exit:\n");
            print_process_info(info);
        }
    }
}

/**
 * @brief 测试缓存统计
 */
void test_cache_stats()
{
    printf("\n=== Testing Cache Statistics ===\n");
    
    int total, alive, expired;
    linx_process_cache_stats(&total, &alive, &expired);
    
    printf("Cache Statistics:\n");
    printf("  Total processes: %d\n", total);
    printf("  Alive processes: %d\n", alive);
    printf("  Expired processes: %d\n", expired);
}

/**
 * @brief 测试获取所有进程
 */
void test_get_all_processes()
{
    printf("\n=== Testing Get All Processes ===\n");
    
    process_info_t *list;
    int count;
    
    if (linx_process_cache_get_all(&list, &count) == 0) {
        printf("Total processes in cache: %d\n", count);
        
        /* 打印前5个进程信息 */
        int show_count = count > 5 ? 5 : count;
        for (int i = 0; i < show_count; i++) {
            printf("\nProcess %d/%d:\n", i + 1, count);
            print_process_info(&list[i]);
        }
        
        if (count > 5) {
            printf("\n... and %d more processes\n", count - 5);
        }
        
        free(list);
    } else {
        printf("Failed to get process list\n");
    }
}

/**
 * @brief 实时监控模式
 */
void monitor_mode()
{
    printf("\n=== Entering Monitor Mode (Press Ctrl+C to stop) ===\n");
    
    while (g_running) {
        system("clear");
        
        printf("Process Cache Monitor - %s\n", ctime(&(time_t){time(NULL)}));
        printf("========================================\n");
        
        /* 显示统计信息 */
        int total, alive, expired;
        linx_process_cache_stats(&total, &alive, &expired);
        
        printf("Statistics:\n");
        printf("  Total: %d | Alive: %d | Expired: %d\n\n", total, alive, expired);
        
        /* 显示进程列表 */
        process_info_t *list;
        int count;
        
        if (linx_process_cache_get_all(&list, &count) == 0) {
            printf("%-8s %-8s %-8s %-12s %-20s %s\n",
                   "PID", "PPID", "State", "Memory(MB)", "Command", "Status");
            printf("--------------------------------------------------------------------------------\n");
            
            for (int i = 0; i < count && i < 20; i++) {
                const char *state_str = "?";
                switch (list[i].state) {
                    case PROCESS_STATE_RUNNING: state_str = "R"; break;
                    case PROCESS_STATE_SLEEPING: state_str = "S"; break;
                    case PROCESS_STATE_ZOMBIE: state_str = "Z"; break;
                    case PROCESS_STATE_STOPPED: state_str = "T"; break;
                    case PROCESS_STATE_EXITED: state_str = "X"; break;
                }
                
                printf("%-8d %-8d %-8s %-12.2f %-20.20s %s\n",
                       list[i].pid, list[i].ppid, state_str,
                       list[i].rss / (1024.0 * 1024.0),
                       list[i].comm,
                       list[i].is_alive ? "alive" : "exited");
            }
            
            if (count > 20) {
                printf("... and %d more processes\n", count - 20);
            }
            
            free(list);
        }
        
        sleep(2);
    }
}

int main(int argc, char *argv[])
{
    printf("Process Cache Test Program\n");
    printf("==========================\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化进程缓存 */
    printf("Initializing process cache...\n");
    if (linx_process_cache_init() < 0) {
        fprintf(stderr, "Failed to initialize process cache\n");
        return 1;
    }
    
    /* 等待缓存初始化完成 */
    sleep(2);
    
    if (argc > 1 && strcmp(argv[1], "monitor") == 0) {
        /* 监控模式 */
        monitor_mode();
    } else {
        /* 测试模式 */
        test_basic_functions();
        test_child_process();
        test_cache_stats();
        test_get_all_processes();
        
        /* 手动更新测试 */
        printf("\n=== Testing Manual Update ===\n");
        pid_t pid = getpid();
        if (linx_process_cache_update(pid) == 0) {
            printf("Manual update for PID %d successful\n", pid);
        } else {
            printf("Manual update failed\n");
        }
        
        /* 清理测试 */
        printf("\n=== Testing Cleanup ===\n");
        int cleaned = linx_process_cache_cleanup();
        printf("Cleaned %d expired entries\n", cleaned);
    }
    
    /* 销毁进程缓存 */
    printf("\nDestroying process cache...\n");
    linx_process_cache_destroy();
    
    printf("Test completed.\n");
    return 0;
}