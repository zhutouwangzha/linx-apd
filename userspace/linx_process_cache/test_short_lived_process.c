#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#include "linx_process_cache.h"

static void test_short_lived_process(void)
{
    pid_t child_pid;
    int status;
    linx_process_info_t *info;
    int found_before_exit = 0;
    int found_after_exit = 0;
    
    printf("测试短暂进程缓存功能...\n");
    
    /* 创建一个短暂的子进程 */
    child_pid = fork();
    if (child_pid == 0) {
        /* 子进程：执行一个很短的任务后退出 */
        usleep(100000); /* 100ms */
        exit(42);
    } else if (child_pid > 0) {
        /* 父进程：检查缓存 */
        
        /* 稍等一下，让事件处理完成 */
        usleep(50000); /* 50ms */
        
        /* 检查进程是否被缓存 */
        info = linx_process_cache_get(child_pid);
        if (info) {
            printf("✓ 成功缓存短暂进程 PID=%d (生命周期中)\n", child_pid);
            printf("  进程名: %s\n", info->comm);
            printf("  父PID: %d\n", info->ppid);
            printf("  状态: %d\n", info->state);
            found_before_exit = 1;
        } else {
            printf("✗ 未能缓存短暂进程 PID=%d (生命周期中)\n", child_pid);
        }
        
        /* 等待子进程退出 */
        waitpid(child_pid, &status, 0);
        
        /* 再次检查缓存，进程应该已标记为退出但仍然缓存 */
        usleep(100000); /* 100ms，等待事件处理 */
        
        info = linx_process_cache_get(child_pid);
        if (info) {
            printf("✓ 进程退出后仍然缓存 PID=%d\n", child_pid);
            printf("  状态: %d (应该是已退出状态)\n", info->state);
            printf("  存活标志: %d (应该是0)\n", info->is_alive);
            if (info->exit_time > 0) {
                printf("  退出时间已记录: %ld\n", info->exit_time);
            }
            found_after_exit = 1;
        } else {
            printf("✗ 进程退出后未能保留缓存 PID=%d\n", child_pid);
        }
        
    } else {
        perror("fork failed");
        return;
    }
    
    /* 测试结果统计 */
    printf("\n测试结果:\n");
    if (found_before_exit && found_after_exit) {
        printf("✓ 短暂进程缓存测试通过\n");
    } else {
        printf("✗ 短暂进程缓存测试失败\n");
        printf("  生命周期中找到: %s\n", found_before_exit ? "是" : "否");
        printf("  退出后找到: %s\n", found_after_exit ? "是" : "否");
    }
}

static void test_rapid_fork(void)
{
    printf("\n测试快速fork多个短暂进程...\n");
    
    int num_processes = 10;
    pid_t pids[num_processes];
    int found_count = 0;
    
    /* 快速创建多个短暂进程 */
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程：立即退出 */
            usleep(10000 + i * 1000); /* 10-19ms */
            exit(i);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork failed");
            pids[i] = -1;
        }
    }
    
    /* 等待一段时间让事件处理完成 */
    usleep(200000); /* 200ms */
    
    /* 检查每个进程是否被缓存 */
    for (int i = 0; i < num_processes; i++) {
        if (pids[i] > 0) {
            linx_process_info_t *info = linx_process_cache_get(pids[i]);
            if (info) {
                found_count++;
                printf("✓ 缓存进程 %d/%d: PID=%d\n", i+1, num_processes, pids[i]);
            } else {
                printf("✗ 未缓存进程 %d/%d: PID=%d\n", i+1, num_processes, pids[i]);
            }
        }
    }
    
    /* 等待所有子进程退出 */
    for (int i = 0; i < num_processes; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }
    
    printf("\n快速fork测试结果: %d/%d 进程被成功缓存\n", found_count, num_processes);
    if (found_count >= num_processes * 0.8) { /* 允许80%的成功率 */
        printf("✓ 快速fork测试通过\n");
    } else {
        printf("✗ 快速fork测试失败\n");
    }
}

static void show_cache_stats(void)
{
    int total, alive, expired;
    linx_process_cache_stats(&total, &alive, &expired);
    
    printf("\n当前缓存统计:\n");
    printf("  总进程数: %d\n", total);
    printf("  存活进程: %d\n", alive);
    printf("  已退出进程: %d\n", expired);
}

int main(void)
{
    printf("启动进程缓存模块...\n");
    
    if (linx_process_cache_init() != 0) {
        fprintf(stderr, "进程缓存初始化失败\n");
        return 1;
    }
    
    printf("缓存模块初始化成功\n");
    
    /* 等待一段时间让初始扫描完成 */
    sleep(2);
    
    show_cache_stats();
    
    /* 运行测试 */
    test_short_lived_process();
    test_rapid_fork();
    
    show_cache_stats();
    
    /* 清理 */
    printf("\n清理缓存模块...\n");
    linx_process_cache_deinit();
    
    printf("测试完成\n");
    return 0;
}