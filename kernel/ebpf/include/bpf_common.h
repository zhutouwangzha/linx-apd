#ifndef __BPF_COMMON_H__
#define __BPF_COMMON_H__

#include "vmlinux/vmlinux.h"
#include "missing_defines.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_endian.h>

#define LINX_READ_TASK_FIELD_INTO(dst, src, a, ...)                                         \
    ({                                                                                      \
        if (bpf_core_enum_value_exists(enum bpf_func_id, BPF_FUNC_get_current_task_btf) &&  \
            (bpf_core_enum_value(enum bpf_func_id, BPF_FUNC_get_current_task_btf) ==        \
            BPF_FUNC_get_current_task_btf))                                                 \
        {                                                                                   \
            *dst = ___arrow((src), a, ##__VA_ARGS__);                                       \
        } else {                                                                            \
            BPF_CORE_READ_INTO(dst, src, a, ##__VA_ARGS__);                                 \
        }                                                                                   \
    })

#endif /* __BPF_COMMON_H__ */
