#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(epoll_pwait_e, struct pt_regs *regs, long id)
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
int BPF_PROG(epoll_pwait_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int epfd */
    int32_t __epfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __epfd);

    /* struct epoll_event * events */
    uint64_t __events = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __events);

    /* int maxevents */
    int32_t __maxevents = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __maxevents);

    /* int timeout */
    int32_t __timeout = (int32_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_s32(ringbuf, __timeout);

    /* const sigset_t * sigmask */
    uint64_t __sigmask = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __sigmask);

    /* size_t sigsetsize */
    uint64_t __sigsetsize = (uint64_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u64(ringbuf, __sigsetsize);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
