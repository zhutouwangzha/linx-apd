#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(fanotify_init_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FANOTIFY_INIT_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(fanotify_init_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FANOTIFY_INIT_X, ret);

    /* unsigned int flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u32(ringbuf, __flags);

    /* unsigned int event_f_flags */
    uint32_t __event_f_flags = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __event_f_flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
