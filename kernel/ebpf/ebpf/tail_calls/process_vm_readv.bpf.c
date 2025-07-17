#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(process_vm_readv_e, struct pt_regs *regs, long id)
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
int BPF_PROG(process_vm_readv_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* pid_t pid */
    int32_t __pid = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __pid);

    /* const struct iovec * lvec */
    uint64_t __lvec = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __lvec);

    /* unsigned long liovcnt */
    uint64_t __liovcnt = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __liovcnt);

    /* const struct iovec * rvec */
    uint64_t __rvec = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __rvec);

    /* unsigned long riovcnt */
    uint64_t __riovcnt = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __riovcnt);

    /* unsigned long flags */
    uint64_t __flags = (uint64_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u64(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
