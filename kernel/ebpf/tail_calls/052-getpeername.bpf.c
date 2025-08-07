#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(getpeername_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETPEERNAME_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(getpeername_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETPEERNAME_X, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* struct sockaddr * usockaddr */
    uint64_t __usockaddr = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __usockaddr);

    /* int * usockaddr_len */
    int32_t *__usockaddr_len = (int32_t *)get_pt_regs_argumnet(regs, 2);
    int32_t ___usockaddr_len = 0;
    if (__usockaddr_len) { 
        bpf_probe_read_user(&___usockaddr_len, sizeof(___usockaddr_len), __usockaddr_len);
    }
    linx_ringbuf_store_s32(ringbuf, ___usockaddr_len);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
