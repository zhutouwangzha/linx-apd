#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(sendfile_e, struct pt_regs *regs, long id)
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
int BPF_PROG(sendfile_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int out_fd */
    int32_t __out_fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __out_fd);

    /* int in_fd */
    int32_t __in_fd = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __in_fd);

    /* loff_t * offset */
    int64_t *__offset = (int64_t *)get_pt_regs_argumnet(regs, 2);
    int64_t ___offset = 0;
    if (__offset) { 
        bpf_probe_read_user(&___offset, sizeof(___offset), __offset);
    }
    linx_ringbuf_store_s64(ringbuf, ___offset);

    /* size_t count */
    uint64_t __count = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __count);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
