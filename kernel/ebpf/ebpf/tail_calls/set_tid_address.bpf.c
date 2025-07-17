#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(set_tid_address_e, struct pt_regs *regs, long id)
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
int BPF_PROG(set_tid_address_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int * tidptr */
    int32_t *__tidptr = (int32_t *)get_pt_regs_argumnet(regs, 0);
    int32_t ___tidptr = 0;
    if (__tidptr) { 
        bpf_probe_read_user(&___tidptr, sizeof(___tidptr), __tidptr);
    }
    linx_ringbuf_store_s32(ringbuf, ___tidptr);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
