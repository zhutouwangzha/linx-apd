#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(getresgid_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETRESGID_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(getresgid_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_GETRESGID_X, ret);

    /* gid_t * rgidp */
    uint32_t *__rgidp = (uint32_t *)get_pt_regs_argumnet(regs, 0);
    uint32_t ___rgidp = 0;
    if (__rgidp) { 
        bpf_probe_read_user(&___rgidp, sizeof(___rgidp), __rgidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___rgidp);

    /* gid_t * egidp */
    uint32_t *__egidp = (uint32_t *)get_pt_regs_argumnet(regs, 1);
    uint32_t ___egidp = 0;
    if (__egidp) { 
        bpf_probe_read_user(&___egidp, sizeof(___egidp), __egidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___egidp);

    /* gid_t * sgidp */
    uint32_t *__sgidp = (uint32_t *)get_pt_regs_argumnet(regs, 2);
    uint32_t ___sgidp = 0;
    if (__sgidp) { 
        bpf_probe_read_user(&___sgidp, sizeof(___sgidp), __sgidp);
    }
    linx_ringbuf_store_u32(ringbuf, ___sgidp);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
