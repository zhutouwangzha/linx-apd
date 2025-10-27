#ifndef __LINX_PROCESS_CACHE_H__
#define __LINX_PROCESS_CACHE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "uthash.h"
#include "linx_thread_pool.h"

#define PROCESS_CACHE_UPDATE_INTERVAL   1        /* 缓存更新间隔(秒) */
#define PROCESS_CACHE_EXPIRE_TIME       60       /* 进程退出后缓存保留时间(秒) */
#define PROCESS_CACHE_THREAD_NUM        4        /* 线程池线程数 */
#define PROC_PATH_MAX                   256      /* /proc路径最大长度 */
#define PROC_CMDLINE_MAX                4096     /* cmdline最大长度 */
#define PROC_COMM_MAX                   256      /* comm最大长度 */

/* 进程状态枚举 */
typedef enum {
    PROCESS_STATE_RUNNING = 0,     /* 运行中 */
    PROCESS_STATE_SLEEPING,        /* 睡眠中 */
    PROCESS_STATE_ZOMBIE,          /* 僵尸进程 */
    PROCESS_STATE_STOPPED,         /* 停止 */
    PROCESS_STATE_EXITED,          /* 已退出 */
    PROCESS_STATE_UNKNOWN          /* 未知状态 */
} process_state_t;

/* 进程信息结构体 */
typedef struct process_info {
    pid_t pid;                     /* 进程ID */
    pid_t ppid;                    /* 父进程ID */
    pid_t pgid;                    /* 进程组ID */
    pid_t sid;                     /* 会话ID */
    uid_t uid;                     /* 用户ID */
    gid_t gid;                     /* 组ID */
    
    char comm[PROC_COMM_MAX];      /* 进程名(短) */
    char cmdline[PROC_CMDLINE_MAX];/* 命令行 */
    char exe_path[PROC_PATH_MAX];  /* 可执行文件路径 */
    char cwd[PROC_PATH_MAX];       /* 当前工作目录 */
    
    process_state_t state;         /* 进程状态 */
    int nice;                      /* nice值 */
    int priority;                  /* 优先级 */
    unsigned long vsize;           /* 虚拟内存大小 */
    unsigned long rss;             /* 常驻内存大小 */
    unsigned long shared;          /* 共享内存大小 */
    
    unsigned long utime;           /* 用户态CPU时间 */
    unsigned long stime;           /* 内核态CPU时间 */
    unsigned long start_time;      /* 进程启动时间 */
    
    time_t create_time;            /* 缓存创建时间 */
    time_t update_time;            /* 最后更新时间 */
    time_t exit_time;              /* 进程退出时间 */
    int is_alive;                  /* 进程是否存活 */
    
    UT_hash_handle hh;             /* uthash句柄 */
} process_info_t;

/* 进程缓存管理器 */
typedef struct process_cache {
    process_info_t *hash_table;    /* 进程信息哈希表 */
    pthread_rwlock_t lock;         /* 读写锁 */
    linx_thread_pool_t *thread_pool; /* 线程池 */
    int running;                   /* 运行状态 */
    pthread_t monitor_thread;      /* 监控线程 */
    pthread_t cleaner_thread;      /* 清理线程 */
} process_cache_t;

/* 初始化进程缓存 */
int linx_process_cache_init(void);

/* 销毁进程缓存 */
void linx_process_cache_destroy(void);

/* 获取进程信息 */
process_info_t *linx_process_cache_get(pid_t pid);

/* 手动更新指定进程缓存 */
int linx_process_cache_update(pid_t pid);

/* 删除指定进程缓存 */
int linx_process_cache_delete(pid_t pid);

/* 获取所有进程列表 */
int linx_process_cache_get_all(process_info_t **list, int *count);

/* 清理过期缓存 */
int linx_process_cache_cleanup(void);

/* 获取缓存统计信息 */
void linx_process_cache_stats(int *total, int *alive, int *expired);

#endif /* __LINX_PROCESS_CACHE_H__ */