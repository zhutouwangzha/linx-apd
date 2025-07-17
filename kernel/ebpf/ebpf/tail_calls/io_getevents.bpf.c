#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(io_getevents_e, struct pt_regs *regs, long id)
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
int BPF_PROG(io_getevents_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* aio_context_t ctx_id */
    uint64_t __ctx_id = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __ctx_id);

    /* long min_nr */
    int64_t __min_nr = (int64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s64(ringbuf, __min_nr);

    /* long nr */
    int64_t __nr = (int64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s64(ringbuf, __nr);

    /* struct io_event * events */
    uint64_t __events = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __events);

    /* struct __kernel_timespec * timeout */
    uint64_t __timeout = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __timeout);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
