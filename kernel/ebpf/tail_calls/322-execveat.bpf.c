#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(execveat_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_EXECVEAT_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(execveat_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_EXECVEAT_X, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* const char * filename */
    uint64_t __filename = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, __filename, LINX_CHARBUF_MAX_SIZE, USER);

    /* const char *const * argv */
    uint64_t __argv = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_charpointer(ringbuf, __argv, LINX_CHARBUF_MAX_SIZE, USER);

    /* const char *const * envp */
    uint64_t __envp = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_charpointer(ringbuf, __envp, LINX_CHARBUF_MAX_SIZE, USER);

    /* int flags */
    int32_t __flags = (int32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_s32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
