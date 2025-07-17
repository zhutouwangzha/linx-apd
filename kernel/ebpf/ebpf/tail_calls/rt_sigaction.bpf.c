#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(rt_sigaction_e, struct pt_regs *regs, long id)
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
int BPF_PROG(rt_sigaction_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int sig */
    int32_t __sig = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __sig);

    /* const struct sigaction * act */
    uint64_t __act = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __act);

    /* struct sigaction * oact */
    uint64_t __oact = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __oact);

    /* size_t sigsetsize */
    uint64_t __sigsetsize = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __sigsetsize);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
