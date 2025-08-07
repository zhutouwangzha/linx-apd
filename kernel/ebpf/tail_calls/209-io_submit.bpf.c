#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(io_submit_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_IO_SUBMIT_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(io_submit_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_IO_SUBMIT_X, ret);

    /* aio_context_t ctx_id */
    uint64_t __ctx_id = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __ctx_id);

    /* long nr */
    int64_t __nr = (int64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s64(ringbuf, __nr);

    /* struct iocb * * iocbpp */
    uint64_t __iocbpp = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __iocbpp);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
