#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(semget_e, struct pt_regs *regs, long id)
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
int BPF_PROG(semget_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* key_t key */
    int32_t __key = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __key);

    /* int nsems */
    int32_t __nsems = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __nsems);

    /* int semflg */
    int32_t __semflg = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __semflg);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
