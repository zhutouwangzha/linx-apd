#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(mmap_e, struct pt_regs *regs, long id)
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
int BPF_PROG(mmap_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* unsigned long addr */
    uint64_t __addr = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __addr);

    /* unsigned long len */
    uint64_t __len = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __len);

    /* unsigned long prot */
    uint64_t __prot = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __prot);

    /* unsigned long flags */
    uint64_t __flags = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __flags);

    /* unsigned long fd */
    uint64_t __fd = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __fd);

    /* unsigned long off */
    uint64_t __off = (uint64_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u64(ringbuf, __off);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
