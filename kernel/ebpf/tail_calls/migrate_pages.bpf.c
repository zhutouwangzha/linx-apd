#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(migrate_pages_e, struct pt_regs *regs, long id)
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
int BPF_PROG(migrate_pages_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* pid_t pid */
    int32_t __pid = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __pid);

    /* unsigned long maxnode */
    uint64_t __maxnode = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __maxnode);

    /* const unsigned long * old_nodes */
    uint64_t *__old_nodes = (uint64_t *)get_pt_regs_argumnet(regs, 2);
    uint64_t ___old_nodes = 0;
    if (__old_nodes) { 
        bpf_probe_read_user(&___old_nodes, sizeof(___old_nodes), __old_nodes);
    }
    linx_ringbuf_store_u64(ringbuf, ___old_nodes);

    /* const unsigned long * new_nodes */
    uint64_t *__new_nodes = (uint64_t *)get_pt_regs_argumnet(regs, 3);
    uint64_t ___new_nodes = 0;
    if (__new_nodes) { 
        bpf_probe_read_user(&___new_nodes, sizeof(___new_nodes), __new_nodes);
    }
    linx_ringbuf_store_u64(ringbuf, ___new_nodes);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
