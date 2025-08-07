#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(fsconfig_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FSCONFIG_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(fsconfig_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_FSCONFIG_X, ret);

    /* int fd */
    int32_t __fd = (int32_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_s32(ringbuf, __fd);

    /* unsigned int cmd */
    uint32_t __cmd = (uint32_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_u32(ringbuf, __cmd);

    /* const char * _key */
    uint64_t ___key = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_charpointer(ringbuf, ___key, LINX_CHARBUF_MAX_SIZE, USER);

    /* const void * _value */
    uint64_t ___value = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, ___value);

    /* int aux */
    int32_t __aux = (int32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_s32(ringbuf, __aux);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
