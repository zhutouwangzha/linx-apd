#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(sendto_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SENDTO_E, -1);

    unsigned long args[5] = {0};
    extract__network_args(args, 5, regs);

    /* fd */
    int32_t socket_fd = (int32_t)args[0];
    linx_ringbuf_store_s64(ringbuf, (int64_t)socket_fd);

    /* size */
    uint32_t size = (uint32_t)args[2];
    linx_ringbuf_store_u32(ringbuf, size);

    /* tuple */
    if (socket_fd >= 0) {
        struct sockaddr *usrsockaddr = (struct sockaddr *)args[4];
        linx_ringbuf_store_socktuple(ringbuf, socket_fd, OUTBOUND, usrsockaddr);
    } else {
        linx_ringbuf_store_empty(ringbuf);
    }

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(sendto_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SENDTO_X, ret);

    /* res */
    linx_ringbuf_store_s64(ringbuf, ret);

    unsigned long args[3] = {0};
    extract__network_args(args, 3, regs);

    snaplen_args_t snaplen_args = {
        .only_port_range = false,
        .type = LINX_EVENT_TYPE_SENDTO_X,
    };

    int64_t bytes_to_read = ret > 0 ? ret : args[2];
    uint16_t snaplen = maps_get_snaplen();
    apply_snaplen(regs, &snaplen, &snaplen_args);
    if ((int64_t)snaplen > bytes_to_read) {
        snaplen = bytes_to_read;
    }

    // 确保不会超出缓冲区边界
    uint64_t available_space = LINX_EVENT_MAX_SIZE - ringbuf->payload_pos;
    if (available_space <= 0) {
        snaplen = 0;
    } else if (snaplen >= available_space) {
        snaplen = available_space - 1;
    }
    
    // 额外的安全检查：确保snaplen不会导致越界访问
    if (snaplen > LINX_EVENT_MAX_SIZE) {
        snaplen = LINX_EVENT_MAX_SIZE;
    }

    /* data */
    unsigned long sent_data_pointer = args[1];
    linx_ringbuf_store_bytebuf(ringbuf, sent_data_pointer, snaplen, USER);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
