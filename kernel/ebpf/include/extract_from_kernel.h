#ifndef __EXTRACT_FROM_KERNEL_H__
#define __EXTRACT_FROM_KERNEL_H__ 

#include "bpf_common.h"

#define PAGE_SHIFT(x) (x) << (IOC_PAGE_SHIFT - 10)

static inline dev_t encode_dev(dev_t dev)
{
    unsigned int major = MAJOR(dev);
    unsigned int minor = MINOR(dev);

    return (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
}

static inline unsigned long extract__vm_size(struct mm_struct *mm)
{
    unsigned long vm_pages = 0;
    BPF_CORE_READ_INTO(&vm_pages, mm, total_vm);
    return PAGE_SHIFT(vm_pages);
}

static inline unsigned long extract__vm_rss(struct mm_struct *mm)
{
    int64_t file_pages = 0;
    int64_t anon_pages = 0;
    int64_t shmem_pages = 0;

    BPF_CORE_READ_INTO(&file_pages, mm, rss_stat[MM_FILEPAGES].count);
    BPF_CORE_READ_INTO(&anon_pages, mm, rss_stat[MM_ANONPAGES].count);
    BPF_CORE_READ_INTO(&shmem_pages, mm, rss_stat[MM_SHMEMPAGES].count);

    return PAGE_SHIFT(file_pages + anon_pages + shmem_pages);
}

static inline uint32_t extract__tty(struct task_struct *task)
{
    struct signal_struct *signal;
    struct tty_struct *tty;
    struct tty_driver *driver;
    int major = 0, minor_start = 0, index = 0;

    BPF_CORE_READ_INTO(&signal, task, signal);
    if (!signal) {
        return 0;
    }

    BPF_CORE_READ_INTO(&tty, signal, tty);
    if (!tty) {
        return 0;
    }

    BPF_CORE_READ_INTO(&driver, tty, driver);
    if (!driver) {
        return 0;
    }

    BPF_CORE_READ_INTO(&index, tty, index);
    BPF_CORE_READ_INTO(&major, driver, major);
    BPF_CORE_READ_INTO(&minor_start, driver, minor_start);

    return encode_dev(MKDEV(major, minor_start) + index);
}

static inline void extract__loginuid(struct task_struct *task, uint32_t *loginuid)
{
    *loginuid = __UINT32_MAX__;

    LINX_READ_TASK_FIELD_INTO(loginuid, task, loginuid.val);
}

static inline struct pid *extract__task_pid_struct(struct task_struct *task)
{
    struct pid *task_pid = NULL;

    LINX_READ_TASK_FIELD_INTO(&task_pid, task, thread_pid);

    return task_pid;
}

static inline struct pid_namespace *extract__namespace_of_pid(struct pid *pid)
{
    uint32_t level = 0;
    struct pid_namespace *ns = NULL;

    if (pid) {
        BPF_CORE_READ_INTO(&level, pid, level);
        BPF_CORE_READ_INTO(&ns, pid, numbers[level].ns);
    }

    return ns;
}

static inline pid_t extract__xid_nr_seen_by_namespace(struct pid *pid, struct pid_namespace *ns)
{
    struct upid upid = {0};
    pid_t nr = 0;
    unsigned int pid_level = 0, ns_level = 0;

    BPF_CORE_READ_INTO(&pid_level, pid, level);
    BPF_CORE_READ_INTO(&ns_level, ns, level);

    if (pid && ns_level <= pid_level) {
        BPF_CORE_READ_INTO(&upid, pid, numbers[ns_level]);

        if (upid.ns == ns) {
            nr = upid.nr;
        }
    }

    return nr;
}

static inline pid_t extract__task_xid_vnr(struct task_struct *task)
{
    struct pid *pid_struct = extract__task_pid_struct(task);
    struct pid_namespace *pid_ns = extract__namespace_of_pid(pid_struct);
    return extract__xid_nr_seen_by_namespace(pid_struct, pid_ns);
}

static inline void extract__network_args(void *argv, int num, struct pt_regs *regs)
{
    unsigned long *dst = (unsigned long *)argv;

    for (int i = 0; i < num; ++i) {
        dst[i] = get_pt_regs_argumnet(regs, i);
    }
}

static inline struct file *extract__file_struct_from_fd(int32_t file)
{
    struct file *f = NULL;
    struct task_struct *task;

    if (file >= 0) {
        struct file **fds = NULL;
        struct fdtable *fdt = NULL;
        int max_fds = 0;

        task = bpf_get_current_task_btf();
        
        BPF_CORE_READ_INTO(&fdt, task, files, fdt);
        if (unlikely(fdt == NULL)) {
            return NULL;
        }

        BPF_CORE_READ_INTO(&max_fds, fdt, max_fds);
        if (unlikely(file >= max_fds)) {
            return NULL;
        }

        BPF_CORE_READ_INTO(&fds, fdt, fd);
        if (fds != NULL) {
            bpf_probe_read_kernel(&f, sizeof(struct file *), &fds[file]);
        }
    }

    return f;
}

static inline struct socket *get_sock_from_file(struct file *file)
{
    if (file == NULL) {
        return NULL;
    }

    // struct file_operations *f_op = (struct file_operations *)BPF_CORE_READ(file, f_op);

    return (struct socket *)BPF_CORE_READ(file, private_data);
}

static inline void extract__dev_ino_overlay_from_fd(int32_t fd, dev_t *dev, uint64_t *ino)
{
    struct file *f = extract__file_struct_from_fd(fd);
    if (!f) {
        return;
    }

    struct inode *i = BPF_CORE_READ(f, f_inode);

    BPF_CORE_READ_INTO(dev, i, i_sb, s_dev);
    *dev = encode_dev(*dev);
    BPF_CORE_READ_INTO(ino, i, i_ino);
}

#endif /* __EXTRACT_FROM_KERNEL_H__ */
