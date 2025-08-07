#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(futex_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FUTEX_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(futex_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FUTEX_X, ret);

    /* u32 * uaddr */
    uint32_t *__uaddr = (uint32_t *)get_pt_regs_argumnet(regs, 0);
    uint32_t ___uaddr = 0;
    if (__uaddr) { 
        bpf_probe_read_user(&___uaddr, sizeof(___uaddr), __uaddr);
    }
    linx_ringbuf_store_u32(ringbuf, ___uaddr);

    /* int op */
    int32_t __op = (int32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_s32(ringbuf, __op);

    /* u32 val */
    uint32_t __val = (uint32_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u32(ringbuf, __val);

    /* const struct __kernel_timespec * utime */
    uint64_t __utime = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __utime);

    /* u32 * uaddr2 */
    uint32_t *__uaddr2 = (uint32_t *)get_pt_regs_argumnet(regs, 4);
    uint32_t ___uaddr2 = 0;
    if (__uaddr2) { 
        bpf_probe_read_user(&___uaddr2, sizeof(___uaddr2), __uaddr2);
    }
    linx_ringbuf_store_u32(ringbuf, ___uaddr2);

    /* u32 val3 */
    uint32_t __val3 = (uint32_t)get_pt_regs_argumnet(regs, 5);
    linx_ringbuf_store_u32(ringbuf, __val3);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
