#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(move_mount_e, struct pt_regs *regs, long id)
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
int BPF_PROG(move_mount_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int from_dfd */
    int32_t __from_dfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __from_dfd);

    /* const char * from_pathname */
    uint64_t __from_pathname = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __from_pathname, LINX_CHARBUF_MAX_SIZE, USER);

    /* int to_dfd */
    int32_t __to_dfd = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __to_dfd);

    /* const char * to_pathname */
    uint64_t __to_pathname = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_charpointer(ringbuf, __to_pathname, LINX_CHARBUF_MAX_SIZE, USER);

    /* unsigned int flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
