#ifndef __LINX_PROCESS_CACHE_INFO_H__
#define __LINX_PROCESS_CACHE_INFO_H__ 

#include <sys/types.h>
#include <stdbool.h>

#include "uthash.h"

#include "linx_process_cache_define.h"
#include "linx_process_state.h"

typedef struct {
    pid_t pid;                              /* 进程ID */
    pid_t ppid;                             /* 父进程ID */
    pid_t pgid;                             /* 进程组ID */
    uint32_t sid;                           /* 会话ID */
    uint32_t uid;                           /* 用户ID */
    uint32_t gid;                           /* 组ID */
    uint32_t tty;                           /* tty */
    uint32_t loginuid;                      /*  */
    uint64_t cmdnargs;                      /* 命令行参数个数 */

    char name[PROC_COMM_MAX_LEN];           /* 进程名 读取 task->comm 或 /proc/pid/comm */
    char cmdline[PROC_CMDLINE_LEN];         /* name + args */
    char exe[PROC_PATH_MAX_LEN];            /* argv[0] 读取 /proc/pid/cmdline */
    char exepath[PROC_PATH_MAX_LEN];        /* 进程的完整可执行路径 读取 /proc/pid/exe */
    char cwd[PROC_PATH_MAX_LEN];            /* 当前工作目录 */
    char args[4096];                        /*  不包括argv[0] */

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
