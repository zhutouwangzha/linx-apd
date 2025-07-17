#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(truncate_e, struct pt_regs *regs, long id)
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
int BPF_PROG(truncate_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* const char * path */
    uint64_t __path = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __path, LINX_CHARBUF_MAX_SIZE, USER);

    /* long length */
    int64_t __length = (int64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s64(ringbuf, __length);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
