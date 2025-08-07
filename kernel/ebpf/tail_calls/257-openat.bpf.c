#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(openat_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPENAT_E, -1);

    int32_t dirfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)dirfd);

    unsigned long path_pointer = get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, path_pointer, LINX_CHARBUF_MAX_SIZE, USER);

    uint32_t flags = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, flags);

    unsigned long mode = get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u32(ringbuf, mode);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(openat_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_OPENAT_X, ret);

    dev_t dev = 0;
    uint64_t ino = 0;

    if (ret > 0) {
        extract__dev_ino_overlay_from_fd(ret, &dev, &ino);
    }

    /* fd */
    linx_ringbuf_store_s64(ringbuf, ret);

    int32_t dirfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)dirfd);

    unsigned long path_pointer = get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, path_pointer, LINX_CHARBUF_MAX_SIZE, USER);

    uint32_t flags = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, flags);

    unsigned long mode = get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u32(ringbuf, mode);

    linx_ringbuf_store_u32(ringbuf, dev);

    linx_ringbuf_store_u64(ringbuf, ino);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
