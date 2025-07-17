#include "../include/kmod_msg.h"

const struct sysmon_event_info g_event_info[] = {
    [SYSMON_GENERIC_E] = {"syscall", 1, {{"id", PT_UINT32}}},
    [SYSMON_GENERIC_X] = {"syscall", 1, {{"res", PT_INT32}}},

    [SYSMON_SYSCALL_OPEN_E] = {"open", 3,
                            {{"name", PT_CHARBUF},
                             {"flags", PT_UINT32},
                             {"mode", PT_INT32}}},
    [SYSMON_SYSCALL_OPEN_X] = {"open", 3,
                            {{"name", PT_CHARBUF},
                             {"flags", PT_UINT32},
                             {"mode", PT_INT32}}},

    [SYSMON_SYSCALL_CLOSE_E] = {"close", 1,
                            {{"fd", PT_INT64}}},
    [SYSMON_SYSCALL_CLOSE_X] = {"close", 1,
                            {{"res", PT_INT32}}},

    [SYSMON_SYSCALL_READ_E] = {"read", 2,
                            {{"fd", PT_INT64},
                             {"size", PT_UINT32}}},
    [SYSMON_SYSCALL_READ_X] = {"read", 3,
                            {{"data", PT_CHARBUF},
                             {"fd", PT_INT32},
                             {"size", PT_INT32}}},

    [SYSMON_SYSCALL_WRITE_E] = {"write", 2,
                            {{"fd", PT_INT64},
                             {"size", PT_UINT32}}},
    [SYSMON_SYSCALL_WRITE_X] = {"write", 3,
                             {{"data", PT_CHARBUF},
                             {"fd", PT_INT32},
                             {"size", PT_INT32}}},

    [SYSMON_SYSCALL_UNLINK_E] = {"unlink", 1,
                            {{"path", PT_CHARBUF}}},
    [SYSMON_SYSCALL_UNLINK_X] = {"unlink", 1,
                            {{"res", PT_INT32}}},

    [SYSMON_SYSCALL_UNLINKAT_E] = {"unlinkat", 2,
                            {{"dirfd", PT_INT32},
                             {"path", PT_CHARBUF}}},
    [SYSMON_SYSCALL_UNLINKAT_X] = {"unlinkat", 1,
                            {{"res", PT_INT32}}},
};