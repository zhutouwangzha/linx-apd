#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(lseek_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_LSEEK_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(lseek_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_LSEEK_X, ret);

    /* unsigned int fd */
    uint32_t __fd = (uint32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u32(ringbuf, __fd);

    /* off_t offset */
    int64_t __offset = (int64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s64(ringbuf, __offset);

    /* unsigned int whence */
    uint32_t __whence = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, __whence);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
