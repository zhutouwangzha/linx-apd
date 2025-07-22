# Linux Process Cache Module

## 概述

`linx_process_cache` 是一个高性能的进程信息缓存模块，用于缓存 `/proc` 下的进程相关信息。该模块使用多线程池进行并发更新，使用 uthash 进行高效索引，并支持进程退出后的延迟清理功能。

## 特性

- **多线程并发更新**：使用 `linx_thread_pool` 模块创建线程池，支持并发更新进程信息
- **高效索引**：使用 uthash 哈希表，以 PID 为键快速查找进程信息
- **自动更新**：监控线程定期扫描 `/proc` 目录，自动更新进程信息
- **延迟清理**：进程退出后不立即删除缓存，而是保留一段时间（默认60秒）
- **读写锁保护**：使用读写锁保护缓存数据，支持高并发读取
- **丰富的进程信息**：包括进程状态、内存使用、CPU时间、命令行等详细信息

## 数据结构

### 进程信息结构

```c
typedef struct process_info {
    pid_t pid;                     /* 进程ID */
    pid_t ppid;                    /* 父进程ID */
    pid_t pgid;                    /* 进程组ID */
    pid_t sid;                     /* 会话ID */
    uid_t uid;                     /* 用户ID */
    gid_t gid;                     /* 组ID */
    
    char comm[256];                /* 进程名(短) */
    char cmdline[4096];            /* 命令行 */
    char exe_path[256];            /* 可执行文件路径 */
    char cwd[256];                 /* 当前工作目录 */
    
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
} process_info_t;
```

### 进程状态枚举

```c
typedef enum {
    PROCESS_STATE_RUNNING = 0,     /* 运行中 */
    PROCESS_STATE_SLEEPING,        /* 睡眠中 */
    PROCESS_STATE_ZOMBIE,          /* 僵尸进程 */
    PROCESS_STATE_STOPPED,         /* 停止 */
    PROCESS_STATE_EXITED,          /* 已退出 */
    PROCESS_STATE_UNKNOWN          /* 未知状态 */
} process_state_t;
```

## API 接口

### 初始化和销毁

```c
/* 初始化进程缓存 */
int linx_process_cache_init(void);

/* 销毁进程缓存 */
void linx_process_cache_destroy(void);
```

### 基本操作

```c
/* 获取进程信息 */
process_info_t *linx_process_cache_get(pid_t pid);

/* 手动更新指定进程缓存 */
int linx_process_cache_update(pid_t pid);

/* 删除指定进程缓存 */
int linx_process_cache_delete(pid_t pid);
```

### 批量操作

```c
/* 获取所有进程列表 */
int linx_process_cache_get_all(process_info_t **list, int *count);

/* 清理过期缓存 */
int linx_process_cache_cleanup(void);

/* 获取缓存统计信息 */
void linx_process_cache_stats(int *total, int *alive, int *expired);
```

## 使用示例

### 基本使用

```c
#include "linx_process_cache.h"

int main() {
    /* 初始化缓存 */
    if (linx_process_cache_init() < 0) {
        fprintf(stderr, "Failed to initialize process cache\n");
        return -1;
    }
    
    /* 获取进程信息 */
    pid_t pid = getpid();
    process_info_t *info = linx_process_cache_get(pid);
    if (info) {
        printf("Process %d: %s\n", info->pid, info->comm);
        printf("Memory: %lu KB\n", info->rss / 1024);
    }
    
    /* 获取所有进程 */
    process_info_t *list;
    int count;
    if (linx_process_cache_get_all(&list, &count) == 0) {
        printf("Total processes: %d\n", count);
        free(list);
    }
    
    /* 销毁缓存 */
    linx_process_cache_destroy();
    return 0;
}
```

### 监控进程变化

```c
/* 监控子进程 */
pid_t child = fork();
if (child == 0) {
    /* 子进程逻辑 */
    exit(0);
} else {
    /* 等待缓存更新 */
    sleep(2);
    
    /* 获取子进程信息 */
    process_info_t *info = linx_process_cache_get(child);
    if (info) {
        if (info->is_alive) {
            printf("Child process is running\n");
        } else {
            printf("Child process exited at %s", ctime(&info->exit_time));
        }
    }
}
```

## 配置参数

可以通过修改头文件中的宏定义来调整模块行为：

- `PROCESS_CACHE_UPDATE_INTERVAL`: 缓存更新间隔（默认1秒）
- `PROCESS_CACHE_EXPIRE_TIME`: 进程退出后缓存保留时间（默认60秒）
- `PROCESS_CACHE_THREAD_NUM`: 线程池线程数（默认4）

## 编译和测试

```bash
# 编译
make

# 运行测试
make test

# 运行监控模式
make monitor

# 清理
make clean
```

## 注意事项

1. **权限问题**：某些进程信息需要相应权限才能读取
2. **性能考虑**：大量进程时可能占用较多内存
3. **线程安全**：所有API都是线程安全的
4. **内存管理**：`linx_process_cache_get_all` 返回的列表需要调用者释放

## 依赖

- `linx_thread_pool`: 线程池模块
- `uthash`: 哈希表实现
- POSIX线程库（pthread）

## 作者

Linux Process Cache Module