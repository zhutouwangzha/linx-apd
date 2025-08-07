#ifndef __LINX_EVENT_H__
#define __LINX_EVENT_H__

#include "linx_size_define.h"

typedef struct {
    uint64_t    tid;                                            /* tid */
    uint64_t    pid;                                            /* pid */
    uint64_t    ppid;                                           /* ppid */
    uint64_t    uid;                                            /* uid */
    uint64_t    gid;                                            /* gid */
    uint64_t    time;                                           /* 进入系统调用的时间 */
    uint64_t    res;                                            /* 返回值 */
    uint32_t    type;                                           /* 标识事件类型 */
    uint32_t    nparams;                                        /* 参数个数 */
    uint32_t    nfds;                                           /* 当前有效的fd个数 */
    uint32_t    fds[LINX_FDS_MAX_SIZE];                         /* 和当前任务关联的fd */
    char        fd_path[LINX_FDS_MAX_SIZE][LINX_PATH_MAX_SIZE]; /* fd对应的路径 */
    char        comm[LINX_COMM_MAX_SIZE];                       /* 执行的命令 */
    char        cmdline[LINX_CMDLINE_MAX_SIZE];                 /* 执行的命令 */
    char        fullpath[LINX_PATH_MAX_SIZE];                   /* 命令的绝对路径 */
    char        p_fullpath[LINX_PATH_MAX_SIZE];                 /* 父进程的绝对路径 */
    uint64_t    params_size[SYSCALL_PARAMS_MAX_COUNT];          /* 参数的长度 */
    uint64_t    size;                                           /* 事件总大小(头+参数) */
} linx_event_t;

#endif /* __LINX_EVENT_H__ */
