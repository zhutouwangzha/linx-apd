#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(get_mempolicy_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GET_MEMPOLICY_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(get_mempolicy_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GET_MEMPOLICY_X, ret);

    /* int * policy */
    int32_t *__policy = (int32_t *)get_pt_regs_argumnet(regs, 0);
    int32_t ___policy = 0;
    if (__policy) { 
        bpf_probe_read_user(&___policy, sizeof(___policy), __policy);
    }
    linx_ringbuf_store_s32(ringbuf, ___policy);

    /* unsigned long * nmask */
    uint64_t *__nmask = (uint64_t *)get_pt_regs_argumnet(regs, 1);
    uint64_t ___nmask = 0;
    if (__nmask) { 
        bpf_probe_read_user(&___nmask, sizeof(___nmask), __nmask);
    }
    linx_ringbuf_store_u64(ringbuf, ___nmask);

    /* unsigned long maxnode */
    uint64_t __maxnode = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __maxnode);

    /* unsigned long addr */
    uint64_t __addr = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __addr);

    /* unsigned long flags */
    uint64_t __flags = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
