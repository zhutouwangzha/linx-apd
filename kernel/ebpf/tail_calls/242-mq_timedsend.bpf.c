#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(mq_timedsend_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_MQ_TIMEDSEND_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(mq_timedsend_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_MQ_TIMEDSEND_X, ret);

    /* mqd_t mqdes */
    int32_t __mqdes = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __mqdes);

    /* const char * u_msg_ptr */
    uint64_t __u_msg_ptr = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __u_msg_ptr, LINX_CHARBUF_MAX_SIZE, USER);

    /* size_t msg_len */
    uint64_t __msg_len = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __msg_len);

    /* unsigned int msg_prio */
    uint32_t __msg_prio = (uint32_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u32(ringbuf, __msg_prio);

    /* const struct __kernel_timespec * u_abs_timeout */
    uint64_t __u_abs_timeout = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __u_abs_timeout);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
