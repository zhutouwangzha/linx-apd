#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(msgsnd_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_MSGSND_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(msgsnd_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_MSGSND_X, ret);

    /* int msqid */
    int32_t __msqid = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __msqid);

    /* struct msgbuf * msgp */
    uint64_t __msgp = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __msgp);

    /* size_t msgsz */
    uint64_t __msgsz = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __msgsz);

    /* int msgflg */
    int32_t __msgflg = (int32_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_s32(ringbuf, __msgflg);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
