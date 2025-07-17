#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(splice_e, struct pt_regs *regs, long id)
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
int BPF_PROG(splice_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int fd_in */
    int32_t __fd_in = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd_in);

    /* loff_t * off_in */
    int64_t *__off_in = (int64_t *)get_pt_regs_argumnet(regs, 1);
    int64_t ___off_in = 0;
    if (__off_in) { 
        bpf_probe_read_user(&___off_in, sizeof(___off_in), __off_in);
    }
    linx_ringbuf_store_s64(ringbuf, ___off_in);

    /* int fd_out */
    int32_t __fd_out = (int32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_s32(ringbuf, __fd_out);

    /* loff_t * off_out */
    int64_t *__off_out = (int64_t *)get_pt_regs_argumnet(regs, 3);
    int64_t ___off_out = 0;
    if (__off_out) { 
        bpf_probe_read_user(&___off_out, sizeof(___off_out), __off_out);
    }
    linx_ringbuf_store_s64(ringbuf, ___off_out);

    /* size_t len */
    uint64_t __len = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __len);

    /* unsigned int flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
