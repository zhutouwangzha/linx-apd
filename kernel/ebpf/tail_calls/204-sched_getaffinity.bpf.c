#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(sched_getaffinity_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SCHED_GETAFFINITY_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(sched_getaffinity_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SCHED_GETAFFINITY_X, ret);

    /* pid_t pid */
    int32_t __pid = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __pid);

    /* unsigned int len */
    uint32_t __len = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __len);

    /* unsigned long * user_mask_ptr */
    uint64_t *__user_mask_ptr = (uint64_t *)get_pt_regs_argumnet(regs, 2);
    uint64_t ___user_mask_ptr = 0;
    if (__user_mask_ptr) { 
        bpf_probe_read_user(&___user_mask_ptr, sizeof(___user_mask_ptr), __user_mask_ptr);
    }
    linx_ringbuf_store_u64(ringbuf, ___user_mask_ptr);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
