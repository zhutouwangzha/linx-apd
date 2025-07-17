#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(ioctl_e, struct pt_regs *regs, long id)
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
int BPF_PROG(ioctl_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* unsigned int fd */
    uint32_t __fd = (uint32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u32(ringbuf, __fd);

    /* unsigned int cmd */
    uint32_t __cmd = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __cmd);

    /* unsigned long arg */
    uint64_t __arg = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __arg);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
