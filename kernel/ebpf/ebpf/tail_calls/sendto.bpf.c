#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(sendto_e, struct pt_regs *regs, long id)
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
int BPF_PROG(sendto_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* void * buff */
    uint64_t __buff = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __buff);

    /* size_t len */
    uint64_t __len = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __len);

    /* unsigned int flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u32(ringbuf, __flags);

    /* struct sockaddr * addr */
    uint64_t __addr = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __addr);

    /* int addr_len */
    int32_t __addr_len = (int32_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_s32(ringbuf, __addr_len);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
