#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#include "linx_process_cache.h"

static void test_ultra_short_process(void)
{
    pid_t child_pid;
    int status;
    linx_process_info_t *info;
    int found_before_exit = 0;
    int found_after_exit = 0;
    
    printf("测试极短进程缓存功能（10ms生命周期）...\n");
    
    /* 创建一个极短的子进程 */
    child_pid = fork();
    if (child_pid == 0) {
        /* 子进程：执行极短任务后退出 */
        usleep(10000); /* 仅10ms */
        exit(42);
    } else if (child_pid > 0) {
        /* 父进程：检查缓存 */
        
        /* 稍等一下，让高频扫描检测到 */
        usleep(20000); /* 20ms */
        
        /* 检查进程是否被缓存 */
        info = linx_process_cache_get(child_pid);
        if (info) {
            printf("✓ 成功缓存极短进程 PID=%d (生命周期10ms)\n", child_pid);
            printf("  进程名: %s\n", info->comm);
            printf("  父PID: %d\n", info->ppid);
            printf("  状态: %d\n", info->state);
            found_before_exit = 1;
        } else {
            printf("✗ 未能缓存极短进程 PID=%d (生命周期10ms)\n", child_pid);
        }
        
        /* 等待子进程退出 */
        waitpid(child_pid, &status, 0);
        
        /* 再次检查缓存，进程应该已标记为退出但仍然缓存 */
        usleep(50000); /* 50ms，等待状态更新 */
        
        info = linx_process_cache_get(child_pid);
        if (info) {
            printf("✓ 进程退出后仍然缓存 PID=%d\n", child_pid);
            printf("  状态: %d (应该是已退出状态)\n", info->state);
            printf("  存活标志: %d (应该是0)\n", info->is_alive);
            if (info->exit_time > 0) {
                printf("  退出时间已记录: %ld\n", info->exit_time);
                printf("  生命周期: %ld 秒\n", info->exit_time - info->create_time);
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
    printf("\n极短进程测试结果:\n");
    if (found_before_exit && found_after_exit) {
        printf("✓ 极短进程缓存测试通过\n");
    } else {
        printf("✗ 极短进程缓存测试失败\n");
        printf("  生命周期中找到: %s\n", found_before_exit ? "是" : "否");
        printf("  退出后找到: %s\n", found_after_exit ? "是" : "否");
    }
}

static void test_rapid_burst_processes(void)
{
    printf("\n测试爆发式短进程创建...\n");
    
    int num_processes = 20;
    pid_t pids[num_processes];
    int found_count = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    /* 快速创建大量短进程 */
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程：非常短的生命周期 */
            usleep(5000 + i * 500); /* 5-15ms */
            exit(i);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork failed");
            pids[i] = -1;
        }
        usleep(1000); /* 1ms间隔创建 */
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    long creation_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 + 
                           (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    
    printf("创建 %d 个进程耗时: %ld ms\n", num_processes, creation_time_ms);
    
    /* 等待一段时间让高频扫描完成 */
    usleep(100000); /* 100ms */
    
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
    
    printf("\n爆发式短进程测试结果: %d/%d 进程被成功缓存 (%.1f%%)\n", 
           found_count, num_processes, (double)found_count * 100.0 / num_processes);
    
    if (found_count >= num_processes * 0.9) { /* 期望90%以上的成功率 */
        printf("✓ 爆发式短进程测试通过\n");
    } else {
        printf("✗ 爆发式短进程测试失败\n");
    }
}

static void test_high_freq_toggle(void)
{
    printf("\n测试高频模式开关...\n");
    
    /* 禁用高频模式 */
    linx_process_cache_set_high_freq_mode(0);
    sleep(1);
    
    /* 创建一个短进程测试 */
    pid_t pid = fork();
    if (pid == 0) {
        usleep(5000); /* 5ms */
        exit(0);
    } else if (pid > 0) {
        usleep(20000); /* 20ms */
        
        linx_process_info_t *info = linx_process_cache_get(pid);
        if (!info) {
            printf("✓ 禁用高频模式时未缓存到短进程（预期行为）\n");
        } else {
            printf("? 禁用高频模式时仍缓存到短进程\n");
        }
        
        int status;
        waitpid(pid, &status, 0);
    }
    
    /* 重新启用高频模式 */
    linx_process_cache_set_high_freq_mode(1);
    sleep(1);
    
    /* 再次测试 */
    pid = fork();
    if (pid == 0) {
        usleep(5000); /* 5ms */
        exit(0);
    } else if (pid > 0) {
        usleep(20000); /* 20ms */
        
        linx_process_info_t *info = linx_process_cache_get(pid);
        if (info) {
            printf("✓ 重新启用高频模式后成功缓存短进程\n");
        } else {
            printf("✗ 重新启用高频模式后仍未缓存短进程\n");
        }
        
        int status;
        waitpid(pid, &status, 0);
    }
}

static void show_detailed_stats(void)
{
    int total, alive, expired;
    long scanned, short_lived, cycles;
    
    linx_process_cache_get_detailed_stats(&total, &alive, &expired, 
                                        &scanned, &short_lived, &cycles);
    
    printf("\n详细缓存统计:\n");
    printf("  总进程数: %d\n", total);
    printf("  存活进程: %d\n", alive);
    printf("  已退出进程: %d\n", expired);
    printf("  总扫描次数: %ld\n", scanned);
    printf("  短暂进程数: %ld\n", short_lived);
    printf("  扫描周期数: %ld\n", cycles);
    
    if (cycles > 0) {
        printf("  平均每周期扫描: %.1f 个进程\n", (double)scanned / cycles);
    }
}

int main(void)
{
    printf("启动高频进程缓存模块测试...\n");
    
    if (linx_process_cache_init() != 0) {
        fprintf(stderr, "进程缓存初始化失败\n");
        return 1;
    }
    
    printf("缓存模块初始化成功\n");
    
    /* 等待一段时间让初始扫描完成 */
    sleep(3);
    
    show_detailed_stats();
    
    /* 运行测试 */
    test_ultra_short_process();
    test_rapid_burst_processes();
    test_high_freq_toggle();
    
    show_detailed_stats();
    
    /* 清理 */
    printf("\n清理缓存模块...\n");
    linx_process_cache_deinit();
    
    printf("测试完成\n");
    return 0;
}