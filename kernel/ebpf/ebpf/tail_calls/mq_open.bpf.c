#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(mq_open_e, struct pt_regs *regs, long id)
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
int BPF_PROG(mq_open_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* const char * u_name */
    uint64_t __u_name = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __u_name, LINX_CHARBUF_MAX_SIZE, USER);

    /* int oflag */
    int32_t __oflag = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __oflag);

    /* umode_t mode */
    uint16_t __mode = (uint16_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u16(ringbuf, __mode);

    /* struct mq_attr * u_attr */
    uint64_t __u_attr = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __u_attr);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
