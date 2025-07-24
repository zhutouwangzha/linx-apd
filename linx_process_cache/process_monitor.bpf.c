#include <linux/bpf.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* 事件类型 */
#define PROCESS_EVENT_FORK  1
#define PROCESS_EVENT_EXEC  2
#define PROCESS_EVENT_EXIT  3

/* 进程事件结构 */
struct process_event {
    __u32 event_type;
    __u32 pid;
    __u32 ppid;
    __u32 tgid;
    __u32 uid;
    __u32 gid;
    __u64 start_time;
    __u64 exit_time;
    __u32 exit_code;
    char comm[16];
    char filename[256];
};

/* 定义perf event array map */
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
    __uint(max_entries, 1024);
} events SEC(".maps");

/* 定义进程启动时间存储 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __u32);        /* PID */
    __type(value, __u64);      /* start time */
    __uint(max_entries, 10240);
} process_start_times SEC(".maps");

/* 获取当前时间戳（纳秒） */
static __always_inline __u64 get_time_ns(void)
{
    return bpf_ktime_get_ns();
}

/* 发送事件到用户空间 */
static __always_inline void send_event(struct process_event *event)
{
    bpf_perf_event_output(bpf_get_current_task_struct(), &events, BPF_F_CURRENT_CPU,
                         event, sizeof(*event));
}

/* 监控进程fork事件 */
SEC("tracepoint/syscalls/sys_enter_clone")
int trace_clone_enter(struct trace_event_raw_sys_enter *ctx)
{
    struct process_event event = {};
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    
    event.event_type = PROCESS_EVENT_FORK;
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.tgid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event.uid = bpf_get_current_uid_gid() >> 32;
    event.gid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event.start_time = get_time_ns();
    
    /* 获取父进程PID */
    if (task) {
        struct task_struct *parent;
        bpf_core_read(&parent, sizeof(parent), &task->real_parent);
        if (parent) {
            bpf_core_read(&event.ppid, sizeof(event.ppid), &parent->pid);
        }
    }
    
    /* 获取进程名 */
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    send_event(&event);
    
    return 0;
}

/* 监控进程fork退出事件 */
SEC("tracepoint/syscalls/sys_exit_clone")
int trace_clone_exit(struct trace_event_raw_sys_exit *ctx)
{
    long ret = ctx->ret;
    
    if (ret > 0) {
        /* fork成功，记录子进程启动时间 */
        __u32 child_pid = (__u32)ret;
        __u64 start_time = get_time_ns();
        bpf_map_update_elem(&process_start_times, &child_pid, &start_time, BPF_ANY);
    }
    
    return 0;
}

/* 监控进程exec事件 */
SEC("tracepoint/syscalls/sys_enter_execve")
int trace_execve_enter(struct trace_event_raw_sys_enter *ctx)
{
    struct process_event event = {};
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    const char __user *filename_ptr = (const char __user *)ctx->args[0];
    
    event.event_type = PROCESS_EVENT_EXEC;
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.tgid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event.uid = bpf_get_current_uid_gid() >> 32;
    event.gid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event.start_time = get_time_ns();
    
    /* 获取父进程PID */
    if (task) {
        struct task_struct *parent;
        bpf_core_read(&parent, sizeof(parent), &task->real_parent);
        if (parent) {
            bpf_core_read(&event.ppid, sizeof(event.ppid), &parent->pid);
        }
    }
    
    /* 获取进程名 */
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    /* 获取可执行文件路径 */
    if (filename_ptr) {
        bpf_probe_read_user_str(event.filename, sizeof(event.filename), filename_ptr);
    }
    
    send_event(&event);
    
    return 0;
}

/* 监控进程退出事件 */
SEC("tracepoint/sched/sched_process_exit")
int trace_sched_process_exit(struct trace_event_raw_sched_process_template *ctx)
{
    struct process_event event = {};
    __u32 pid = ctx->pid;
    __u64 *start_time_ptr;
    
    event.event_type = PROCESS_EVENT_EXIT;
    event.pid = pid;
    event.tgid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event.uid = bpf_get_current_uid_gid() >> 32;
    event.gid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event.exit_time = get_time_ns();
    
    /* 查找进程启动时间 */
    start_time_ptr = bpf_map_lookup_elem(&process_start_times, &pid);
    if (start_time_ptr) {
        event.start_time = *start_time_ptr;
        /* 删除记录 */
        bpf_map_delete_elem(&process_start_times, &pid);
    }
    
    /* 获取进程名 */
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    send_event(&event);
    
    return 0;
}

/* 监控进程终止信号 */
SEC("tracepoint/signal/signal_deliver")
int trace_signal_deliver(struct trace_event_raw_signal_deliver *ctx)
{
    int sig = ctx->sig;
    
    /* 只处理终止信号 */
    if (sig == 9 || sig == 15 || sig == 2) {  /* SIGKILL, SIGTERM, SIGINT */
        struct process_event event = {};
        
        event.event_type = PROCESS_EVENT_EXIT;
        event.pid = bpf_get_current_pid_tgid() >> 32;
        event.tgid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
        event.uid = bpf_get_current_uid_gid() >> 32;
        event.gid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
        event.exit_time = get_time_ns();
        event.exit_code = sig;
        
        /* 获取进程名 */
        bpf_get_current_comm(&event.comm, sizeof(event.comm));
        
        send_event(&event);
    }
    
    return 0;
}

char _license[] SEC("license") = "GPL";
__u32 _version SEC("version") = LINUX_VERSION_CODE;