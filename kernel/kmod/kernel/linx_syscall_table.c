
#include <asm/unistd.h>
#include "../include/kmod_msg.h"

const struct syscall_evt_pair g_syscall_table[] = {
    [__NR_open] = {SC_UF_USED | SC_UF_NEVER_DROP, SYSMON_SYSCALL_OPEN_E, SYSMON_SYSCALL_OPEN_X},
    [__NR_openat] = {SC_UF_USED | SC_UF_NEVER_DROP, SYSMON_SYSCALL_OPEN_E, SYSMON_SYSCALL_OPEN_X},
    [__NR_unlink] = {SC_UF_USED | SC_UF_NEVER_DROP, SYSMON_SYSCALL_UNLINK_E, SYSMON_SYSCALL_UNLINK_X},
    [__NR_unlinkat] = {SC_UF_USED | SC_UF_NEVER_DROP, SYSMON_SYSCALL_UNLINKAT_E, SYSMON_SYSCALL_UNLINKAT_X},
};