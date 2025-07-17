#ifndef __LINX_EVENT_H__
#define __LINX_EVENT_H__

#include <linux/types.h>
#include "kmod_msg.h"
#include "linx_consumer.h"


typedef struct event_filler_arguments {
    struct linx_kmod_consumer *consumer;
    uint32_t syscall_nr;
    char *buffer; 
    uint32_t curarg;
    uint32_t nargs;
    uint32_t arg_data_offset;
    uint32_t arg_data_size;
    sysmon_event_code event_type;
    struct pt_regs *regs;
    unsigned long args[6];
    uint32_t args_len[8];

}event_filler_arguments_t;

extern const struct sysmon_event_entry g_sysmon_events[];

#endif /* __LINX_EVENTS_H__ */

