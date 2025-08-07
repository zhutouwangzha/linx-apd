#ifndef __BPF_CHECK_H__
#define __BPF_CHECK_H__

#include "maps.h"

static inline int check_pid_need_filtered(uint32_t pid)
{
    for (int i = 0; i < LINX_BPF_FILTER_PID_MAX_SIZE && g_filter_pids[i]; ++i) {
        if (pid == g_filter_pids[i]) {
            return -1;
        }
    }

    return 0;
}

static inline int check_comm_need_filtered(const char *comm)
{
    int flag = 0;

    for (int i = 0; 
         i < LINX_BPF_FILTER_COMM_MAX_SIZE &&
         g_filter_comms[i][0] != 0; 
         ++i)
    {
        for (int j = 0; j < LINX_COMM_MAX_SIZE; ++j) {
            if (comm[j] != g_filter_comms[i][j]) {
                flag = 0;
                break;
            } else {
                flag = 1;
            }
        }

        if (flag) {
            return -1;
        }
    }

    return 0;
}

static inline int check_drop_mode(void)
{
    return g_drop_mode;
}

static inline int check_drop_failed(void)
{
    return g_drop_failed;
}

static inline int check_interesting_syscall(uint32_t syscall_id)
{
    if (syscall_id < 0 || syscall_id >= LINX_SYSCALL_ID_MAX) {
        return 0;
    }

    return (int)g_interesting_syscalls_table[syscall_id];
}

#endif /* __BPF_CHECK_H__ */
