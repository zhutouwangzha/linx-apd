#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(read_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_READ_E, -1);

    int32_t fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, fd);

    uint32_t size = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, size);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(read_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_READ_X, ret);

    linx_ringbuf_store_s64(ringbuf, ret);

    if (ret > 0) {
        // snaplen_args_t snaplen_args = {
        //     .only_port_range = false,
        //     .type = LINX_EVENT_TYPE_READ_X,
        // };
        uint16_t snaplen = 512;
        // apply_snaplen(regs, &snaplen, &snaplen_args);
        // if (snaplen > ret) {
        //     snaplen = ret;
        // }

        unsigned long data_pointer = get_pt_regs_argumnet(regs, 1);
        linx_ringbuf_store_bytebuf(ringbuf, data_pointer, snaplen, USER);
    } else {
        linx_ringbuf_store_empty(ringbuf);
    }

    /* unsigned int fd */
    int32_t fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)fd);

    /* size_t count */
    uint32_t size = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, size);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
