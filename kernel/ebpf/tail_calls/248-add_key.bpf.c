#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(add_key_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_ADD_KEY_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(add_key_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_ADD_KEY_X, ret);

    /* const char * _type */
    uint64_t ___type = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, ___type, LINX_CHARBUF_MAX_SIZE, USER);

    /* const char * _description */
    uint64_t ___description = (uint64_t)get_pt_regs_argumnet(regs, 1);
    linx_ringbuf_store_charpointer(ringbuf, ___description, LINX_CHARBUF_MAX_SIZE, USER);

    /* const void * _payload */
    uint64_t ___payload = (uint64_t)get_pt_regs_argumnet(regs, 2);
    linx_ringbuf_store_u64(ringbuf, ___payload);

    /* size_t plen */
    uint64_t __plen = (uint64_t)get_pt_regs_argumnet(regs, 3);
    linx_ringbuf_store_u64(ringbuf, __plen);

    /* key_serial_t ringid */
    int32_t __ringid = (int32_t)get_pt_regs_argumnet(regs, 4);
    linx_ringbuf_store_s32(ringbuf, __ringid);


    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
