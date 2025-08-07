#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(chown_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_CHOWN_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(chown_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_CHOWN_X, ret);

    /* const char * filename */
    uint64_t __filename = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __filename, LINX_CHARBUF_MAX_SIZE, USER);

    /* uid_t user */
    uint32_t __user = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __user);

    /* gid_t group */
    uint32_t __group = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, __group);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
