#ifndef __LINX_PROCESS_CACHE_INFO_H__
#define __LINX_PROCESS_CACHE_INFO_H__ 

#include <sys/types.h>
#include <stdbool.h>

#include "uthash.h"

#include "linx_process_cache_define.h"
#include "linx_process_state.h"

/**
 * 计划有这些字段
 * pid  产生事件的进程ID
 * exe  第一个命令行参数（通常是可执行文件名称或自定义名称）
 * name 生成事件的可执行文件的名称（不包括路径）
 * args 当启动生成事件的进程时，在命令行上传递的参数
 * env  生成事件的进程的环境变量
 * cwd  事件的当前工作目录
 * ppid 事件进程的父进程PID
 * pname 父进程的名称（不包括路径）
 * pcmdline 
 * tty  进程的控制终端。对于没有终端的进程，该值为0
*/
typedef struct {
    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    pid_t sid;
    pid_t uid;
    pid_t gid;

    char comm[PROC_COMM_MAX_LEN];
    char cmdline[PROC_CMDLINE_LEN];
    char exe[PROC_PATH_MAX_LEN];
    char cwd[PROC_PATH_MAX_LEN];

    linx_process_state_t state;
    int nice;
    int priority;
    unsigned long vsize;
    unsigned long rss;
    unsigned long shared;

    unsigned long utime;
    unsigned long stime;
    unsigned long start_time;

    time_t create_time;
    time_t update_time;
    time_t exit_time;
    int is_alive;

    UT_hash_handle hh;
} linx_process_info_t;

#endif /* __LINX_PROCESS_CACHE_INFO_H__ */
