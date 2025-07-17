#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(getgroups_e, struct pt_regs *regs, long id)
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
int BPF_PROG(getgroups_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int gidsetsize */
    int32_t __gidsetsize = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __gidsetsize);

    /* gid_t * grouplist */
    uint32_t *__grouplist = (uint32_t *)get_pt_regs_argumnet(regs, 1);
    uint32_t ___grouplist = 0;
    if (__grouplist) { 
        bpf_probe_read_user(&___grouplist, sizeof(___grouplist), __grouplist);
    }
    linx_ringbuf_store_u32(ringbuf, ___grouplist);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
