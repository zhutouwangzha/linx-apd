#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(recvfrom_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_RECVFROM_E, -1);

    unsigned long args[3] = {0};
    extract__network_args(args, 3, regs);

    int32_t socket_fd = (int32_t)args[0];
    linx_ringbuf_store_s64(ringbuf, (int64_t)socket_fd);

    uint32_t size = (uint32_t)args[2];
    linx_ringbuf_store_u32(ringbuf, size);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(recvfrom_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_RECVFROM_X, ret);

    /* ret */
    linx_ringbuf_store_s64(ringbuf, ret);

    if (ret > 0) {
        uint16_t snaplen = 128;
        // if (snaplen > ret) {
        //     snaplen = ret;
        // }

        unsigned long args[5] = {0};
        extract__network_args(args, 5, regs);

        unsigned long received_data_pointer = args[1];
        linx_ringbuf_store_bytebuf(ringbuf, received_data_pointer, snaplen, USER);

        uint32_t socket_fd = (uint32_t)args[0];
        struct sockaddr *usrsockaddr = (struct sockaddr *)args[4];
        linx_ringbuf_store_socktuple(ringbuf, socket_fd, INBOUND, usrsockaddr);
    } else {
        /* data */
        linx_ringbuf_store_empty(ringbuf);

        /* tuple */
        linx_ringbuf_store_empty(ringbuf);
    }

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
