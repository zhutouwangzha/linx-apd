#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(open_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPEN_E, -1);

    unsigned long name_pointer = (unsigned long)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, name_pointer, LINX_CHARBUF_MAX_SIZE, USER);

    uint32_t flags = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, flags);

    unsigned long mode = get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, mode);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(open_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPEN_X, ret);

    dev_t dev = 0;
    uint64_t ino = 0;

    if (ret > 0) {
        extract__dev_ino_overlay_from_fd(ret, &dev, &ino);
    }

    linx_ringbuf_store_s64(ringbuf, ret);

    uint64_t name_pointer = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, name_pointer, LINX_CHARBUF_MAX_SIZE, USER);

    /* int flags */
    uint32_t flags = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, flags);

    /* umode_t mode */
    unsigned long mode = get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, mode);

    linx_ringbuf_store_u32(ringbuf, dev);

    linx_ringbuf_store_u64(ringbuf, ino);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
