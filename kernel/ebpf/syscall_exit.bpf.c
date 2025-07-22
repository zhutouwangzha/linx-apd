#include "bpf_check.h"
#include "get_pt_regs.h"

SEC("tp_btf/sys_exit")
int BPF_PROG(sys_exit, struct pt_regs *regs, long ret)
{
    long syscall_id = get_syscall_id(regs);
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

    if (check_drop_failed() && ret < 0) {
        return 0;
    }

    bpf_printk("4\n");
    bpf_tail_call(ctx, &syscall_exit_tail_table, syscall_id);

    return 0;
}
