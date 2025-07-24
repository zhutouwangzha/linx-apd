#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(exit_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, id, LINX_SYSCALL_TYPE_ENTER, -1);

    /* 存储退出码 */
    int exit_code = (int)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_param_i32(ringbuf, exit_code);
    
    /* 存储当前进程PID */
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    linx_ringbuf_store_param_i32(ringbuf, (int32_t)pid);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(exit_x, struct pt_regs *regs, long ret)
{
    /* exit系统调用不会返回，所以这里通常不会被执行 */
    return 0;
}

SEC("tp_btf/sys_enter")
int BPF_PROG(exit_group_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, id, LINX_SYSCALL_TYPE_ENTER, -1);

    /* 存储退出码 */
    int exit_code = (int)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_param_i32(ringbuf, exit_code);
    
    /* 存储当前进程PID */
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    linx_ringbuf_store_param_i32(ringbuf, (int32_t)pid);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(exit_group_x, struct pt_regs *regs, long ret)
{
    /* exit_group系统调用不会返回，所以这里通常不会被执行 */
    return 0;
}
