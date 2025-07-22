#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(name_to_handle_at_e, struct pt_regs *regs, long id)
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
int BPF_PROG(name_to_handle_at_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* int dfd */
    int32_t __dfd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __dfd);

    /* const char * name */
    uint64_t __name = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __name, LINX_CHARBUF_MAX_SIZE, USER);

    /* struct file_handle * handle */
    uint64_t __handle = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __handle);

    /* int * mnt_id */
    int32_t *__mnt_id = (int32_t *)get_pt_regs_argumnet(regs, 3);
    int32_t ___mnt_id = 0;
    if (__mnt_id) { 
        bpf_probe_read_user(&___mnt_id, sizeof(___mnt_id), __mnt_id);
    }
    linx_ringbuf_store_s32(ringbuf, ___mnt_id);

    /* int flag */
    int32_t __flag = (int32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_s32(ringbuf, __flag);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
