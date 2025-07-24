#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(execve_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, id, LINX_SYSCALL_TYPE_ENTER, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(execve_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* const char * filename */
    uint64_t __filename = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __filename, LINX_CHARBUF_MAX_SIZE, USER);

    /* const char *const * argv */
    uint64_t __argv = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __argv, LINX_CHARBUF_MAX_SIZE, USER);

    /* const char *const * envp */
    uint64_t __envp = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_charpointer(ringbuf, __envp, LINX_CHARBUF_MAX_SIZE, USER);

    /* 如果execve成功(ret == 0)，存储当前进程PID */
    if (ret == 0) {
        pid_t pid = bpf_get_current_pid_tgid() >> 32;
        linx_ringbuf_store_param_i32(ringbuf, (int32_t)pid);
    }

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
