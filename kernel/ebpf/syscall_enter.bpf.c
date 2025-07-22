#include "bpf_check.h"
#include "get_pt_regs.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(sys_enter, struct pt_regs *regs, long syscall_id)
{
    uint32_t pid = bpf_get_current_pid_tgid();
    char comm[LINX_COMM_MAX_SIZE];
    bpf_get_current_comm(&comm, LINX_COMM_MAX_SIZE);

    if (check_pid_need_filtered(pid) ||
        check_comm_need_filtered(comm) ||
        check_drop_mode()) 
    {
        return 0;
    }

    if (!check_interesting_syscall(syscall_id)) {
        return 0;
    }

    bpf_tail_call(ctx, &syscall_enter_tail_table, syscall_id);

    return 0;
}
