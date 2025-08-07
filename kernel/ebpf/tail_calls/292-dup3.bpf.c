#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(dup3_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_DUP3_E, -1);

    int32_t oldfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, oldfd);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(dup3_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_DUP3_X, ret);

    linx_ringbuf_store_s64(ringbuf, ret);

    /* unsigned int oldfd */
    int32_t oldfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)oldfd);

    /* unsigned int newfd */
    int32_t newfd = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s64(ringbuf, (int64_t)newfd);

    /* int flags */
    int32_t flags = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
