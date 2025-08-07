#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG(execve_e, struct pt_regs *regs, long id)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_EXECVE_E, -1);

    /* const char * filename */
    uint64_t __filename = (uint64_t)get_pt_regs_argumnet(regs, 0);
    linx_ringbuf_store_charpointer(ringbuf, __filename, LINX_CHARBUF_MAX_SIZE, USER);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(execve_x, struct pt_regs *regs, long ret)
{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {
        return 0;
    }

    struct task_struct *task = (struct task_struct *)bpf_get_current_task_btf();

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_EXECVE_X, ret);

    unsigned long arg_start_pointer = 0;
    unsigned long arg_end_pointer = 0;

    LINX_READ_TASK_FIELD_INTO(&arg_start_pointer, task, mm, arg_start);
    LINX_READ_TASK_FIELD_INTO(&arg_end_pointer, task, mm, arg_end);

    /* const char * exe */
    linx_ringbuf_store_charpointer(ringbuf, arg_start_pointer, LINX_CHARBUF_MAX_SIZE, USER);

    struct mm_struct *mm = NULL;
    LINX_READ_TASK_FIELD_INTO(&mm, task, mm);

    /* vm_size */
    uint32_t vm_size = extract__vm_size(mm);
    linx_ringbuf_store_u32(ringbuf, vm_size);

    /* vm_rss */
    uint32_t vm_rss = extract__vm_rss(mm);
    linx_ringbuf_store_u32(ringbuf, vm_rss);

    /* comm */
    linx_ringbuf_store_charpointer(ringbuf, (unsigned long)task->comm, LINX_COMM_MAX_SIZE, KERNEL);

    /* tty */
    uint32_t tty = extract__tty(task);
    linx_ringbuf_store_u32(ringbuf, tty);

    /* env */
    linx_ringbuf_store_charbufarray_as_bytebuf(ringbuf, 
                                               arg_start_pointer, 
                                               arg_end_pointer - arg_start_pointer, 
                                               4096);

    /* loginuid */
    uint32_t loginuid;
    extract__loginuid(task, &loginuid);
    linx_ringbuf_store_u32(ringbuf, loginuid);

    /* pgid */
    pid_t pgid = extract__task_xid_vnr(task);
    linx_ringbuf_store_s64(ringbuf, (int64_t)pgid);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}
