#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(dup_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_DUP_E, -1);

    int32_t oldfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)oldfd);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(dup_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_DUP_X, ret);

    /* res */
    linx_ringbuf_store_s64(ringbuf, ret);

    int32_t oldfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)oldfd);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
