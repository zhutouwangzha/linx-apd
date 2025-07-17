#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(mbind_e, struct pt_regs *regs, long id)
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
int BPF_PROG(mbind_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, get_syscall_id(regs), LINX_SYSCALL_TYPE_EXIT, ret);

    /* unsigned long start */
    uint64_t __start = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_u64(ringbuf, __start);

    /* unsigned long len */
    uint64_t __len = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u64(ringbuf, __len);

    /* unsigned long mode */
    uint64_t __mode = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, __mode);

    /* const unsigned long * nmask */
    uint64_t *__nmask = (uint64_t *)get_pt_regs_argumnet(regs, 3);
    uint64_t ___nmask = 0;
    if (__nmask) { 
        bpf_probe_read_user(&___nmask, sizeof(___nmask), __nmask);
    }
    linx_ringbuf_store_u64(ringbuf, ___nmask);

    /* unsigned long maxnode */
    uint64_t __maxnode = (uint64_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_u64(ringbuf, __maxnode);

    /* unsigned int flags */
    uint32_t __flags = (uint32_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u32(ringbuf, __flags);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
