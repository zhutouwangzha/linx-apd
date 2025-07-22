#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(socketpair_e, struct pt_regs *regs, long id)
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
int BPF_PROG(socketpair_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int family */
    int32_t __family = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __family);

    /* int type */
    int32_t __type = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __type);

    /* int protocol */
    int32_t __protocol = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __protocol);

    /* int * usockvec */
    int32_t *__usockvec = (int32_t *)get_pt_regs_argumnet(regs, 3);
    int32_t ___usockvec = 0;
    if (__usockvec) { 
        bpf_probe_read_user(&___usockvec, sizeof(___usockvec), __usockvec);
    }
    linx_ringbuf_store_s32(ringbuf, ___usockvec);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
