#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(open_tree_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPEN_TREE_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(open_tree_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPEN_TREE_X, ret);

    /* int dfd */
    int32_t __dfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __dfd);

    /* const char * filename */
    uint64_t __filename = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __filename, LINX_CHARBUF_MAX_SIZE, USER);

    /* unsigned flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
