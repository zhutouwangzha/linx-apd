#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(write_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_WRITE_E, -1);

    int32_t fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)fd);

    uint32_t size = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, size);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(write_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_WRITE_X, ret);

    linx_ringbuf_store_s64(ringbuf, ret);

    size_t size = (size_t)get_pt_regs_argumnet(regs, 2);
    // int64_t bytes_to_read = ret > 0 ? ret : size;
    uint16_t snaplen = 512;
    // if ((int64_t)snaplen > bytes_to_read) {
    //     snaplen = bytes_to_read;
    // }

    unsigned long data_pointer = get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_bytebuf(ringbuf, data_pointer, snaplen, USER);

    /* unsigned int fd */
    int32_t fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s64(ringbuf, (int64_t)fd);

    linx_ringbuf_store_u32(ringbuf, size);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
