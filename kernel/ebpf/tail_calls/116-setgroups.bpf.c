#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(setgroups_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SETGROUPS_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(setgroups_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SETGROUPS_X, ret);

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
