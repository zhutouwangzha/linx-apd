#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(getcpu_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETCPU_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(getcpu_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETCPU_X, ret);

    /* unsigned * cpup */
    uint32_t *__cpup = (uint32_t *)get_pt_regs_argumnet(regs, 0);
    uint32_t ___cpup = 0;
    if (__cpup) { 
        bpf_probe_read_user(&___cpup, sizeof(___cpup), __cpup);
    }
    linx_ringbuf_store_u32(ringbuf, ___cpup);

    /* unsigned * nodep */
    uint32_t *__nodep = (uint32_t *)get_pt_regs_argumnet(regs, 1);
    uint32_t ___nodep = 0;
    if (__nodep) { 
        bpf_probe_read_user(&___nodep, sizeof(___nodep), __nodep);
    }
    linx_ringbuf_store_u32(ringbuf, ___nodep);

    /* struct getcpu_cache * unused */
    uint64_t __unused = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __unused);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
