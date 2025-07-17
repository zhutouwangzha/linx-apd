#ifndef __LINX_LOGS_H__
#define __LINX_LOGS_H__

#include "kmod_msg.h"
#include "linx_devset.h"

void linx_log_print(event_header_t *evt);
int linx_logs_open(char *filename);
int linx_logs_write(int  fd, event_header_t *evt);
int linx_logs_close(int fd);
int linx_logs_test(void);
void init_syscall_names(void);
void print_print_all_event(kmod_device_t *dev_set);

#endif /*__LINX_LOGS_H__*/