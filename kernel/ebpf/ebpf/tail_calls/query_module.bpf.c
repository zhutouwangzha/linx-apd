#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(query_module_e, struct pt_regs *regs, long id)
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
int BPF_PROG(query_module_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* const char * name */
    uint64_t __name = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __name, LINX_CHARBUF_MAX_SIZE, USER);

    /* int which */
    int32_t __which = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __which);

    /* void * buf */
    uint64_t __buf = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __buf);

    /* size_t bufsize */
    uint64_t __bufsize = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __bufsize);

    /* size_t * ret */
    uint64_t *__ret = (uint64_t *)get_pt_regs_argumnet(regs, 4);
    uint64_t ___ret = 0;
    if (__ret) { 
        bpf_probe_read_user(&___ret, sizeof(___ret), __ret);
    }
    linx_ringbuf_store_u64(ringbuf, ___ret);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
