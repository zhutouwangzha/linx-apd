#ifndef __LINX_EVENTS_H__
#define __LINX_EVENTS_H__

#include <linux/types.h>
#include <linux/socket.h>
#include "linx_events_public.h"
#include "linx_consumer.h"


typedef struct event_filler_arguments {
    struct linx_kmod_consumer *consumer;
    uint32_t syscall_nr;
    int fd;
    char *buffer; 
    uint32_t curarg;
    uint32_t nargs;
    uint32_t arg_data_offset;
    uint32_t arg_data_size;
    linx_event_type_t event_type;
    struct pt_regs *regs;
    char *str_storage;
    unsigned long args[6];
    uint64_t args_len[32];
}event_filler_arguments_t;

extern const struct sysmon_event_entry g_sysmon_events[];
uint16_t fd_to_socktuple(int fd,
                         struct sockaddr *usrsockaddr,
                         int ulen,
                         bool use_userdata,
                         bool is_inbound,
                         char *targetbuf,
                         uint16_t targetbufsize);

int addr_to_kernel(void __user *uaddr, int ulen, struct sockaddr *kaddr);
unsigned long linx_copy_from_user(void *to, const void __user *from, unsigned long n);
#endif /* __LINX_EVENTS_H__ */

