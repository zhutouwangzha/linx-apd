#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(setresgid_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SETRESGID_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(setresgid_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_SETRESGID_X, ret);

    /* gid_t rgid */
    uint32_t __rgid = (uint32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u32(ringbuf, __rgid);

    /* gid_t egid */
    uint32_t __egid = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __egid);

    /* gid_t sgid */
    uint32_t __sgid = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, __sgid);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
