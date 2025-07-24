#ifndef __LINX_PROCESS_CACHE_EBPF_H__
#define __LINX_PROCESS_CACHE_EBPF_H__

#include <linux/types.h>
#include <sys/types.h>

/* eBPF事件类型 */
typedef enum {
    PROCESS_EVENT_FORK = 1,     /* 进程创建(fork/clone) */
    PROCESS_EVENT_EXEC,         /* 进程执行(exec) */
    PROCESS_EVENT_EXIT,         /* 进程退出 */
    PROCESS_EVENT_COMM_CHANGE   /* 进程名变化 */
} process_event_type_t;

/* eBPF进程事件结构体 */
struct process_event {
    __u32 event_type;           /* 事件类型 */
    __u32 pid;                  /* 进程ID */
    __u32 ppid;                 /* 父进程ID */
    __u32 tgid;                 /* 线程组ID */
    __u32 uid;                  /* 用户ID */
    __u32 gid;                  /* 组ID */
    __u64 start_time;           /* 进程启动时间(纳秒) */
    __u64 exit_time;            /* 进程退出时间(纳秒) */
    __u32 exit_code;            /* 退出码 */
    char comm[16];              /* 进程名 */
    char filename[256];         /* 可执行文件路径 */
};

/* eBPF进程监控初始化 */
int linx_process_ebpf_init(void);

/* eBPF进程监控销毁 */
void linx_process_ebpf_destroy(void);

/* 启动eBPF事件消费者线程 */
int linx_process_ebpf_start_consumer(void);

/* 停止eBPF事件消费者线程 */
void linx_process_ebpf_stop_consumer(void);

/* 处理eBPF进程事件的回调函数类型 */
typedef int (*process_event_handler_t)(const struct process_event *event);

/* 注册事件处理器 */
int linx_process_ebpf_register_handler(process_event_handler_t handler);

#endif /* __LINX_PROCESS_CACHE_EBPF_H__ */