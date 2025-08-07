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
    pid_t pid;                          /* 进程ID */
    pid_t ppid;                         /* 父进程ID */
    pid_t pgid;                         /* 进程组ID */
    pid_t sid;                          /* 会话ID */
    pid_t uid;                          /* 用户ID */
    pid_t gid;                          /* 组ID */

    char name[PROC_COMM_MAX_LEN];       /* 进程名 */
    char comm[PROC_COMM_MAX_LEN];       
    char cmdline[PROC_CMDLINE_LEN];     /* 命令行 */
    char exe[PROC_PATH_MAX_LEN];        /* 进程执行文件路径 */
    char cwd[PROC_PATH_MAX_LEN];        /* 当前工作目录 */

    linx_process_state_t state;
    int nice;                           /* nice 值 */
    int priority;                       /* 优先级 */
    unsigned long vsize;                /* 虚拟内存大小 */
    unsigned long rss;                  /* 驻留内存大小 */
    unsigned long shared;               /* 共享内存大小 */

    unsigned long utime;                /* 用户态时间 */
    unsigned long stime;                /* 内核态时间 */
    unsigned long start_time;           /* 进程创建时间 */

    time_t create_time;                 /* 缓存创建时间 */
    time_t update_time;                 /* 最后更新时间 */
    time_t exit_time;                   /* 进程退出时间 */
    bool is_alive;                      /* 进程是否存活 */

    bool is_rich;                       /* 标识为事件丰富创建的缓存 */
    UT_hash_handle hh;
} linx_process_info_t;

#endif /* __LINX_PROCESS_CACHE_INFO_H__ */
