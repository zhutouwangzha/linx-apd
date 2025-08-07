#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(rseq_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_RSEQ_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(rseq_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_RSEQ_X, ret);

    /* struct rseq * rseq */
    uint64_t __rseq = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __rseq);

    /* u32 rseq_len */
    uint32_t __rseq_len = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __rseq_len);

    /* int flags */
    int32_t __flags = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __flags);

    /* u32 sig */
    uint32_t __sig = (uint32_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u32(ringbuf, __sig);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
