#ifndef __LINX_EVENT_GET_H__
#define __LINX_EVENT_GET_H__

#include <stdint.h>

#include "linx_event.h"

int linx_event_get_fullpath(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_time(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_syscall_id(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_syscall_name(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_syscall(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_res(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_comm(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_cmdline(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_pid(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_tid(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_ppid(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_user_id(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_user(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_user_all(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_group_id(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_group(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_group_all(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_fds_id(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_fds_name(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_fds(linx_event_t *event, char *buf, size_t buf_size);

int get_syscall_enter_params(uint64_t syscall_id, int idx, int *byte_write,
                             uint8_t **data, char *buf, size_t buf_size);

int get_syscall_exit_params(uint64_t syscall_id, int idx, int *byte_write,
                            uint8_t **data, char *buf, size_t buf_size);

int linx_event_get_args(linx_event_t *event, char *buf, size_t buf_size);

int linx_event_get_dir(linx_event_t *event, char *buf, size_t buf_size);

#endif /* __LINX_EVENT_GET_H__ */
