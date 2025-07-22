#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(clone_e, struct pt_regs *regs, long id)
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
int BPF_PROG(clone_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* unsigned long clone_flags */
    uint64_t __clone_flags = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __clone_flags);

    /* unsigned long newsp */
    uint64_t __newsp = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __newsp);

    /* int * parent_tidptr */
    int32_t *__parent_tidptr = (int32_t *)get_pt_regs_argumnet(regs, 2);
    int32_t ___parent_tidptr = 0;
    if (__parent_tidptr) { 
        bpf_probe_read_user(&___parent_tidptr, sizeof(___parent_tidptr), __parent_tidptr);
    }
    linx_ringbuf_store_s32(ringbuf, ___parent_tidptr);

    /* int * child_tidptr */
    int32_t *__child_tidptr = (int32_t *)get_pt_regs_argumnet(regs, 3);
    int32_t ___child_tidptr = 0;
    if (__child_tidptr) { 
        bpf_probe_read_user(&___child_tidptr, sizeof(___child_tidptr), __child_tidptr);
    }
    linx_ringbuf_store_s32(ringbuf, ___child_tidptr);

    /* unsigned long tls */
    uint64_t __tls = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __tls);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
