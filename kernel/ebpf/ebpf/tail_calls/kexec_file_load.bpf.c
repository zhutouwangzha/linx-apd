#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(kexec_file_load_e, struct pt_regs *regs, long id)
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
int BPF_PROG(kexec_file_load_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int kernel_fd */
    int32_t __kernel_fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __kernel_fd);

    /* int initrd_fd */
    int32_t __initrd_fd = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __initrd_fd);

    /* unsigned long cmdline_len */
    uint64_t __cmdline_len = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __cmdline_len);

    /* const char * cmdline_ptr */
    uint64_t __cmdline_ptr = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_charpointer(ringbuf, __cmdline_ptr, LINX_CHARBUF_MAX_SIZE, USER);

    /* unsigned long flags */
    uint64_t __flags = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
