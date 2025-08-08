#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "userspace/linx_process_cache/include/linx_process_cache.h"

void print_process_info(linx_process_info_t *info) {
    if (!info) {
        printf("进程信息为空\n");
        return;
    }
    
    printf("=== 进程信息 ===\n");
    printf("PID: %d\n", info->pid);
    printf("PPID: %d\n", info->ppid);
    printf("PGID: %d\n", info->pgid);
    printf("SID: %u\n", info->sid);
    printf("UID: %u\n", info->uid);
    printf("GID: %u\n", info->gid);
    printf("TTY: %u\n", info->tty);
    printf("Login UID: %u\n", info->loginuid);
    printf("命令行参数个数: %lu\n", info->cmdnargs);
    printf("进程名: %s\n", info->name);
    printf("命令行: %s\n", info->cmdline);
    printf("Exe (argv[0]): %s\n", info->exe);
    printf("Exe路径: %s\n", info->exepath);
    printf("当前工作目录: %s\n", info->cwd);
    printf("参数: %s\n", info->args);
    printf("状态: %d\n", info->state);
    printf("是否存活: %s\n", info->is_alive ? "是" : "否");
    printf("是否富化: %s\n", info->is_rich ? "是" : "否");
    printf("虚拟内存: %lu bytes\n", info->vsize);
    printf("驻留内存: %lu bytes\n", info->rss);
    printf("===============\n\n");
}

int main() {
    printf("测试进程缓存实现\n");
    
    // 初始化进程缓存
    if (linx_process_cache_init() != 0) {
        printf("进程缓存初始化失败\n");
        return -1;
    }
    
    // 测试获取当前进程信息
    pid_t current_pid = getpid();
    printf("获取当前进程信息 (PID: %d)\n", current_pid);
    
    linx_process_info_t *info = linx_process_cache_get(current_pid);
    print_process_info(info);
    
    // 测试获取父进程信息
    pid_t parent_pid = getppid();
    printf("获取父进程信息 (PID: %d)\n", parent_pid);
    
    linx_process_info_t *parent_info = linx_process_cache_get(parent_pid);
    print_process_info(parent_info);
    
    // 清理
    linx_process_cache_deinit();
    
    return 0;
}