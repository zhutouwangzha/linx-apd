#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(fsetxattr_e, struct pt_regs *regs, long id)
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
int BPF_PROG(fsetxattr_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* const char * name */
    uint64_t __name = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __name, LINX_CHARBUF_MAX_SIZE, USER);

    /* const void * value */
    uint64_t __value = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __value);

    /* size_t size */
    uint64_t __size = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __size);

    /* int flags */
    int32_t __flags = (int32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_s32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
