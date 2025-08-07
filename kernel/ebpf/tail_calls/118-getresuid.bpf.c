#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(getresuid_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETRESUID_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(getresuid_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETRESUID_X, ret);

    /* uid_t * ruidp */
    uint32_t *__ruidp = (uint32_t *)get_pt_regs_argumnet(regs, 0);
    uint32_t ___ruidp = 0;
    if (__ruidp) { 
        bpf_probe_read_user(&___ruidp, sizeof(___ruidp), __ruidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___ruidp);

    /* uid_t * euidp */
    uint32_t *__euidp = (uint32_t *)get_pt_regs_argumnet(regs, 1);
    uint32_t ___euidp = 0;
    if (__euidp) { 
        bpf_probe_read_user(&___euidp, sizeof(___euidp), __euidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___euidp);

    /* uid_t * suidp */
    uint32_t *__suidp = (uint32_t *)get_pt_regs_argumnet(regs, 2);
    uint32_t ___suidp = 0;
    if (__suidp) { 
        bpf_probe_read_user(&___suidp, sizeof(___suidp), __suidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___suidp);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
