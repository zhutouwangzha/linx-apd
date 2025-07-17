#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(accept_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, id, LINX_SYSCALL_TYPE_ENTER, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(accept_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* struct sockaddr * upeer_sockaddr */
    uint64_t __upeer_sockaddr = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __upeer_sockaddr);

    /* int * upeer_addrlen */
    int32_t *__upeer_addrlen = (int32_t *)get_pt_regs_argumnet(regs, 2);
    int32_t ___upeer_addrlen = 0;
    if (__upeer_addrlen) { 
        bpf_probe_read_user(&___upeer_addrlen, sizeof(___upeer_addrlen), __upeer_addrlen);
    }
    linx_ringbuf_store_s32(ringbuf, ___upeer_addrlen);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
