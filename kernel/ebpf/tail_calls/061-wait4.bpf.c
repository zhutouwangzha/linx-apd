#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(wait4_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_WAIT4_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(wait4_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_WAIT4_X, ret);

    /* pid_t upid */
    int32_t __upid = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __upid);

    /* int * stat_addr */
    int32_t *__stat_addr = (int32_t *)get_pt_regs_argumnet(regs, 1);
    int32_t ___stat_addr = 0;
    if (__stat_addr) { 
        bpf_probe_read_user(&___stat_addr, sizeof(___stat_addr), __stat_addr);
    }
    linx_ringbuf_store_s32(ringbuf, ___stat_addr);

    /* int options */
    int32_t __options = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __options);

    /* struct rusage * ru */
    uint64_t __ru = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __ru);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
