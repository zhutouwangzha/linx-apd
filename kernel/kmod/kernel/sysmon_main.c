#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <asm/syscall.h>
#include <linux/init.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/sched/user.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/atmioc.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <asm/unistd.h>
#include <asm/page.h>
#include <linux/string.h>
#include <uapi/linux/in.h>
#include <net/inet_sock.h>

#include "../include/kmod_msg.h"
#include "linx_consumer.h"
#include "sysmon_events.h"

#define TRACEPOINT_PROBE(probe, args...) static void probe(void *__data, args)

#define DRIVER_DEVICE_NAME  "clinx"
#define CLASS_NAME   "my_class"

// #define BUFFER_SIZE     (8 * 1024 * 1024)   // 每个CPU 8MB缓冲区
#define BUFFER_SIZE     (32 * 1024)   // 每个CPU 8MB缓冲区



struct kmod_ringbuffer_ctx
{
    struct kmod_ringbuffer_info *ring_info;
    char *buffer;           // 缓冲区数据指针
    uint32_t size;          // 缓冲区大小
    atomic_t preempt_count; // 抢占计数器
    bool open;              // 打开标志
};



// 设备结构体
struct linx_kmod_device {
	dev_t dev;              // 设备号
	struct cdev cdev;       // 字符设备
	wait_queue_head_t read_queue;
};




// 文件操作集
static int linx_kmod_open(struct inode *inode, struct file *filp);
static int linx_kmod_release(struct inode *inode, struct file *filp);
static long linx_kmod_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int linx_kmod_mmap(struct file *filp, struct vm_area_struct *vma);


static int linx_init_ringbuffer(struct kmod_ringbuffer_ctx *ring, unsigned long ringlen);
static void linx_free_ringbuffer(struct kmod_ringbuffer_ctx *ring);

static void linx_get_cmdline(struct task_struct *task, char *cmdline, size_t cmdlen);
static void linx_get_fd_name(struct files_struct *files, char (*buf)[FD_NAME_SIZE], size_t rows, int *fd);

// 全局变量
static const struct file_operations g_linx_kmod_fops = {
        .open = linx_kmod_open,
        .release = linx_kmod_release,
        .mmap = linx_kmod_mmap,
        .unlocked_ioctl = linx_kmod_ioctl,
        .owner = THIS_MODULE,
};

static struct class *g_kmod_class;
static struct linx_kmod_device *g_kmod_devs;
static unsigned int g_kmod_numdevs;
static int g_kmod_major;
static unsigned long g_buffer_size = BUFFER_SIZE;
static uint32_t g_tracepoints_flags; // 注册点 位标记

static LIST_HEAD(linx_consumer_list);
static DEFINE_MUTEX(g_consumer_mutex);



static struct linx_kmod_consumer *linx_kmod_find_consumer(struct task_struct *consumer_id) {
	struct linx_kmod_consumer *el = NULL;

	rcu_read_lock();
	list_for_each_entry_rcu(el, &linx_consumer_list, node) {
		if(el->consumer_id == consumer_id) {
			rcu_read_unlock();
			return el;
		}
	}
	rcu_read_unlock();

	return NULL;
}

/**
 * @brief 字符设备open操作：信息初始化
 * 
 * @param inode 
 * @param filp 
 * @return int 
 */
static int linx_kmod_open(struct inode *inode, struct file *filp)
{
    int ring_no = iminor(inode);
    struct linx_kmod_consumer *consumer = NULL;
    struct task_struct *curr_id = current;
    int ret = 0;

    if (ring_no >= g_kmod_numdevs) {
        ret = -ENODEV;
        goto out;
    }

    mutex_lock(&g_consumer_mutex);

    // 查找现有消费者
    consumer = linx_kmod_find_consumer(curr_id);

    // 检查消费者状态
    if (!consumer) {
        // 创建消费者
        consumer = kzalloc(sizeof(*consumer), GFP_KERNEL);
        if (!consumer) {
            ret = -ENOMEM;
            goto out;
        }

        consumer->ring = kzalloc(sizeof(*consumer->ring), GFP_KERNEL);
        if (!consumer->ring) {
            kfree(consumer);
            ret = -ENOMEM;
            goto out;
        }

        if (linx_init_ringbuffer(consumer->ring, g_buffer_size)) {
            kfree(consumer->ring);
            kfree(consumer);
            goto out;
        }

        consumer->consumer_id = current;
        mutex_init(&consumer->lock);
        list_add_rcu(&consumer->node, &linx_consumer_list);
    } else {
        if (consumer->ring->open) {
            mutex_unlock(&g_consumer_mutex);
            return -EBUSY;
        }
    }

    consumer->ring->open = true;
    consumer->consumer_cfg.drop_enabel = DISABLE;
    consumer->consumer_cfg.fds_enabel = DISABLE;
    consumer->consumer_cfg.args_enable = DISABLE;
    filp->private_data = consumer;

out:
    mutex_unlock(&g_consumer_mutex);
    return ret;
}

/**
 * @brief 释放资源
 * 
 * @param inode 
 * @param filp 
 * @return int 
 */
static int linx_kmod_release(struct inode *inode, struct file *filp)
{
    struct linx_kmod_consumer *consumer = filp->private_data;

    mutex_lock(&g_consumer_mutex);
    if (consumer) {
        consumer->ring->open = false;

        // 如果没有打开的实例，释放消费者
        list_del_rcu(&consumer->node);
        synchronize_rcu();
        if (consumer->ring) {
            linx_free_ringbuffer(consumer->ring);
            synchronize_rcu();
        }
        kfree(consumer->ring);
        kfree(consumer);
    }

    mutex_unlock(&g_consumer_mutex);
    return 0;
}

/**
 * @brief 控制配置：配置消费者、使能探针点
 * 
 * @param filp 
 * @param cmd 
 * @param arg 
 * @return long 
 */
static long linx_kmod_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct linx_kmod_consumer *consumer = filp->private_data;
    unsigned long  syscall_nr;
    int ret;
    uint32_t tp_type;

    if (!consumer)
        return -EBADF;

    if (copy_from_user(&syscall_nr, (void __user *)arg, sizeof(syscall_nr)))
        return -EFAULT;
    
    mutex_lock(&g_consumer_mutex);
    switch (cmd)
    {
    case SYSMON_IOCTL_GTE_PID:
    case SYSMON_IOCTL_GET_VTID:
        pid_t vid;
		struct pid *pid;
		struct task_struct *task;
		struct pid_namespace *ns;

		rcu_read_lock();
		pid = find_pid_ns(arg, &init_pid_ns);
		if(!pid) {
			rcu_read_unlock();
			ret = -EINVAL;
			goto cleanup_ioctl;
		}

		task = pid_task(pid, PIDTYPE_PID);
		if(!task) {
			rcu_read_unlock();
			ret = -EINVAL;
			goto cleanup_ioctl;
		}

		ns = ns_of_pid(pid);
		if(!pid) {
			rcu_read_unlock();
			ret = -EINVAL;
			goto cleanup_ioctl;
		}

		if(cmd == SYSMON_IOCTL_GET_VTID)
			vid = task_pid_nr_ns(task, ns);
		else
			vid = task_tgid_nr_ns(task, ns);

		rcu_read_unlock();
		ret = vid;
        break;
        
        break;
    case SYSMON_IOCTL_GTE_CURRENT_TID:
        ret = task_pid_nr(current);
        goto cleanup_ioctl;
        break;
    case SYSMON_IOCTL_GTE_CURRENT_PID:
        ret = task_tgid_nr(current);
		goto cleanup_ioctl;
        break;
    case SYSMON_IOCTL_ENABLE_SYSCALL:
        if (syscall_nr >= MAX_SYSCALLS_NUM) {
            ret = -EINVAL;
			goto cleanup_ioctl;
        }
        
        set_bit(syscall_nr, consumer->syscall_mask);
        ret = 0;
        
        break;
    case SYSMON_IOCTL_DISABLE_SYSCALL:
        if (syscall_nr >= MAX_SYSCALLS_NUM) {
            ret = -EINVAL;
			goto cleanup_ioctl;
        }
        
        clear_bit(syscall_nr, consumer->syscall_mask);
        ret = 0;
        
        break;
    case SYSMON_IOCTL_ENABLE_TP:
        tp_type = (uint32_t)arg;
        if (arg >= SYSMON_TP_MAX) {
            ret = -EINVAL;
			goto cleanup_ioctl;
        }

        g_tracepoints_flags |= 1 << tp_type;
        ret = 0; 
        break;
    case SYSMON_IOCTL_DISABLE_TP:
        tp_type = (uint32_t)arg;
        if (arg >= SYSMON_TP_MAX) {
            ret = -EINVAL;
			goto cleanup_ioctl;
        }

        g_tracepoints_flags &= ~(1 << tp_type);
        ret = 0; 
        break;
    case SYSMON_IOCTL_DISABLE_DROPPING_MODE:
        consumer->consumer_cfg.drop_enabel = DISABLE;
        ret = 0;
        break;
    case SYSMON_IOCTL_ENABLE_DROPPING_MODE:
        consumer->consumer_cfg.drop_enabel = ENABLE;
        ret = 0;
        break;
    case SYSMON_IOCTL_ENABLE_FDS:
        return 0; /* 当前做保护用 */
        consumer->consumer_cfg.fds_enabel = ENABLE;
        ret = 0;
        break;
    case SYSMON_IOCTL_DISABLE_FDS:
        consumer->consumer_cfg.fds_enabel = DISABLE;
        ret = 0;
        break;
    case SYSMON_IOCTL_ENABLE_REAL_ARGS:
        return 0; /* 当前做保护用 */
        consumer->consumer_cfg.args_enable = ENABLE;
        ret = 0;
        break;
    case SYSMON_IOCTL_DISABLE_REAL_ARGS:
        consumer->consumer_cfg.args_enable = DISABLE;
        ret = 0;
        break;
    default:
        mutex_unlock(&g_consumer_mutex);
        return -ENOTTY;
    }

cleanup_ioctl:

    mutex_unlock(&g_consumer_mutex);
    return ret;

}


/**
 * @brief mmap映射。用于内核和应用层信息交互
 * 
 * @param filp 
 * @param vma 
 * @return int 
 */
static int linx_kmod_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct linx_kmod_consumer *consumer = filp->private_data;
    unsigned long len = vma->vm_end - vma->vm_start;
    unsigned long pfn;
    // char *vmalloc_ptr;

    if (!consumer)
        return -EBADF;
    if (len > consumer->ring->size * 2)
        return -EINVAL;
    
    // 映射缓冲区
    if (len <= PAGE_SIZE) {
        // 映射缓冲区信息结构
        unsigned long useraddr = vma->vm_start;
        char *ringinfo = (char *)consumer->ring->ring_info;
        if (!ringinfo) 
            return -ENODEV;

        pfn = vmalloc_to_pfn(ringinfo);
        if (remap_pfn_range(vma, useraddr, pfn, PAGE_SIZE, vma->vm_page_prot))
            return -EAGAIN;

    } else if (len == consumer->ring->size * 2) {
        // 映射收集信息的缓冲区
        unsigned long mlen = len / 2;
        unsigned long useraddr = vma->vm_start;
        int i;

        // 映射第一半
        for (i = 0; i < mlen; i += PAGE_SIZE)
        {
            pfn = vmalloc_to_pfn(consumer->ring->buffer + i);
            if (remap_pfn_range(vma, useraddr, pfn, PAGE_SIZE, vma->vm_page_prot))
                return -EAGAIN;
            useraddr += PAGE_SIZE;
        }

        // 映射第二半(镜像)
        for (i = 0; i < mlen; i += PAGE_SIZE)
        {
            pfn = vmalloc_to_pfn(consumer->ring->buffer + i);
            if (remap_pfn_range(vma, useraddr, pfn, PAGE_SIZE, vma->vm_page_prot))
                return -EAGAIN;
            useraddr += PAGE_SIZE;
        }
    }
    return 0;
}


/**
 * @brief 初始化环形缓冲区
 * 
 * @return int 
 */
static int linx_init_ringbuffer(struct kmod_ringbuffer_ctx *ring, unsigned long ringlen)
{
    ring->buffer = vmalloc(ringlen + 2 * PAGE_SIZE);
    if (!ring->buffer)
        return -ENOMEM;

    ring->ring_info = vmalloc(sizeof(struct kmod_ringbuffer_info));
    if (!ring->ring_info) {
        vfree(ring->buffer);
        return -ENOMEM;
    }
    
    memset(ring->buffer, 0, ringlen + 2 * PAGE_SIZE);
    ring->size = ringlen;
    ring->ring_info->head = 0;
    ring->ring_info->tail = 0;
    ring->ring_info->n_evts = 0;
    ring->ring_info->n_drop_evts = 0;
    atomic_set(&ring->preempt_count, 0);
    ring->open = false;
    
    return 0;
}


static void linx_free_ringbuffer(struct kmod_ringbuffer_ctx *ring)
{
    if (ring->ring_info) {
        vfree(ring->ring_info);
        ring->ring_info = NULL;
    }
    if (ring->buffer) {
        vfree(ring->buffer);
        ring->buffer = NULL;
    }
    ring->size = 0;
}


int linx_get_params(struct event_filler_arguments *args, 
                    struct event_header *evt, char *buf, size_t buflen)
{
    int ret;
    struct syscall_evt_pair *event_pair;
    sysmon_event_code event_type;

    event_pair = (struct syscall_evt_pair *)&g_syscall_table[257];

    if (!(event_pair->flags & SC_UF_USED)) {
        args->arg_data_offset = 0;
        args->arg_data_size = 0;
        return -1;
    }

    if (evt->type == SYSCALL_ENTER)
        event_type = event_pair->enter_event_type;
    else
        event_type = event_pair->exit_event_type;
    args->nargs = g_event_info[event_type].nparams;
    args->arg_data_offset = 0;
    args->buffer = buf;
    args->arg_data_size = buflen;
    args->syscall_nr = evt->syscall_nr;
    
    args->curarg = 0;
    args->event_type = event_type;
    args->nargs = g_event_info[event_type].nparams;
    
    /* 获取实际参数 */
    if(likely(g_sysmon_events[event_type].filler_callback)) {
                ret = g_sysmon_events[event_type].filler_callback(args);
    } else {
        /* 不处理直接返回 */
        args->arg_data_offset = 0;
        args->arg_data_size = 0;
        return -1;
    }

    if (ret)
        return ret;
    
    
    return 0;
}


/**
 * @brief 记录事件到缓冲区
 * 
 * @param consumer 
 * @param evt 
 * @param args 
 * @return int 
 */
static int linx_record_event(struct linx_kmod_consumer *consumer, struct event_header *evt, unsigned long  *args)
{
    struct kmod_ringbuffer_ctx *ring = consumer->ring;
    event_header_t *hdr;
    uint32_t event_size;
    uint32_t free_space;
    uint32_t next;
    struct event_filler_arguments real_args = {};
    komd_thread_info_t thr_info;
    // int i;
    int ret;
    char buf[1024];

    // 获取抢占锁
    if (atomic_inc_return(&ring->preempt_count) != 1){
        atomic_dec(&ring->preempt_count);
        return 0;
    }

    // 计算可用空间
    if (ring->ring_info->tail > ring->ring_info->head)
        free_space = ring->ring_info->tail  - ring->ring_info->head - 1;
    else
        free_space = ring->size + ring->ring_info->tail - ring->ring_info->head - 1;

    /* 获取详细参数 */
    if (consumer->consumer_cfg.args_enable) {
        memcpy(real_args.args, args, sizeof(real_args.args));
        real_args.consumer = consumer;

        ret = linx_get_params(&real_args, evt, buf, sizeof(buf));
        if (ret < 0)
            event_size = sizeof(struct event_header);
        else
            event_size = sizeof(struct event_header) + real_args.arg_data_offset;
    } else {
        /* 只填充头部 */
        event_size = sizeof(struct event_header);
    }

    if (consumer->consumer_cfg.fds_enabel) {
        int *tmp_fd = evt->fd_val;
        memset(&thr_info, 0, sizeof(komd_thread_info_t));
        if (current->files)
            linx_get_fd_name(current->files, thr_info.fd_name, FD_NUM, tmp_fd);
    }

    /* 判断空间剩余 */
    if (free_space < event_size) {
        atomic_dec(&ring->preempt_count);
        ring->ring_info->n_drop_evts++;
        evt->len = 0;
        return 0;
    }

    evt->len = event_size;

    hdr = (event_header_t *)(ring->buffer + ring->ring_info->head);

    /* 填充头部信息 */
    memcpy(hdr, evt, sizeof(event_header_t));

    /* 填充实际参数信息 */
    // if (consumer->consumer_cfg.args_enable) {
    //     memcpy(hdr + sizeof(event_header_t), buf, real_args.arg_data_offset);
    // }

    /* 填充扩展信息 */
    // if (consumer->consumer_cfg.fds_enabel) {
    //     memcpy(hdr + sizeof(event_header_t) + real_args.arg_data_offset, &thr_info, sizeof(komd_thread_info_t));
    // }

    // 更新头指针
    next = ring->ring_info->head + event_size;
    if (next >= ring->size) {
        if (next > ring->size)
            memcpy(ring->buffer, ring->buffer + ring->size, next - ring->size);
        next -= ring->size;
    }

    ring->ring_info->n_evts++;

    // 确保内存写入完成
    smp_wmb();
    ring->ring_info->head = next;

    atomic_dec(&ring->preempt_count);
    return 1;
}

/**
 * @brief 获取应用层命令行
 * 
 * @param task 
 * @param cmdline 
 * @param cmdlen 
 */
static void linx_get_cmdline(struct task_struct *task, char *cmdline, size_t cmdlen)
{
    int len = 0;
    char buf[256]= {0};
    struct pid *pid;
    struct task_struct *p;
    int ret = 0;
    unsigned long addr = 0;
    

    pid = find_vpid(task->pid);
    if (!pid) {
        pr_err("PID not found\n");
        goto empty;
    }

    p = pid_task(pid, PIDTYPE_PID);
    if (!p) {
        pr_err("Task not found\n");
        goto empty;
    }

# if 0
    /* 方式1 */
    if (!task->mm || !task->mm->arg_start)
        return;

    char *cmd_p = (char *)task->mm->arg_start;
    len = task->mm->arg_end - task->mm->arg_start;

    ret = copy_from_user(buf, (const char __user *)cmd_p, len);
    if (ret != 0) {
        printk("copy error\n");    for_each_kernel_tracepoint(visit_tracepoint, NULL);
    compat_set_tracepoint();
        return;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }
    
    buf[len] = '\0';
#endif
    
#if 1
    /* 方式2 */
    rcu_read_lock();
    addr = task->mm->arg_start;
    len = task->mm->arg_end - task->mm->arg_start;
    if (len > cmdlen)
        len = cmdlen - 1;
    ret = (int)access_process_vm(p, addr, buf, len, 0);
    if (ret != len) {
        goto empty;
    }

    // if (len > cmdlen)
    //     len = cmdlen;

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }

    buf[len] = '\0';
    rcu_read_unlock();
    if (len > cmdlen)
        len = cmdlen;
#endif
    strncpy(cmdline, buf, len);
    return;
empty:
    strncpy(cmdline, "<NA>", cmdlen);
}


/**
 * @brief 获取进程操作文件描述符
 * 
 * @param files 
 * @param buf 
 * @param len 
 */
static void linx_get_fd_name(struct files_struct *files, char (*buf)[FD_NAME_SIZE], size_t rows, int *fd)
{
    struct file *file;
    struct fdtable *fdt;
    char *filename;
    size_t i, j = 0;
    int ret = -1;
    char path[64] = {0};
    int tmp_fd[12] = {0};

    rcu_read_lock();
    fdt = files_fdtable(files);

    for (i = 0; i < fdt->max_fds; i++)
    {
        file = fdt->fd[i];
        if(!file)
            continue;
        filename = d_path(&file->f_path, path, sizeof(path));
        if (IS_ERR(filename)) {
            continue;
        }

        // 文件描述符数量大于12
        if (i > rows)
            break;
        fd[j]= i;
        tmp_fd[j]= i;
        strncpy(path, filename, FD_NAME_SIZE);
        if (j < FD_NUM)
            snprintf(buf[j++], FD_NAME_SIZE, "%s", path);
        ret = 0;
    }
    rcu_read_unlock();

    if (ret < 0) {
        strncpy(buf[0], "<NA>", FD_NAME_SIZE);
        return;
    }
    return;
}


/**
 * @brief 跟踪系统调用
 * 
 * @param id 
 * @return int 
 */
static int linx_trace_syscall(long id)
{
    switch (id)
    {
    case __NR_unlink:
    case __NR_unlinkat:
    case __NR_open:
    case __NR_openat:
    // case __NR_write:
    // case __NR_close:
    // case __NR_read:
    // case __NR_execve:
    // case __NR_fork:
    // case __NR_socket:
    // case __NR_connect:
    // case __NR_bind:
    // case __NR_listen:
    // case __NR_sendto:
    // case __NR_sendmsg:
    // case __NR_recvfrom:
    // case __NR_recvmsg:
        return 1;
        break;
    
    default:
        return 0;
        break;
    }
}

static int linx_net_syscall(long id)
{
    switch (id)
    {
    case __NR_socket:
    case __NR_connect:
    case __NR_bind:
    case __NR_listen:
    case __NR_sendto:
    case __NR_sendmsg:
    case __NR_recvfrom:
    case __NR_recvmsg:
        return 1;
        break;
    
    default:
        return 0;
        break;
    }
}

static void linx_get_socketcall_info(struct socket *sock, struct komd_net_info *netinfo)
{
    struct sock *sk = sock->sk;

    if (!sk)
        return;

    netinfo->protocol = sk->sk_protocol;

    if (sk->sk_family == AF_INET) {
        struct inet_sock *inet = inet_sk(sk);

        netinfo->saddr = inet->inet_saddr;
        netinfo->daddr = inet->inet_daddr;
        netinfo->sport = ntohs(inet->inet_sport);
        netinfo->dport = ntohs(inet->inet_dport);
    }
}

static void linx_net_info(event_header_t *evt, struct pt_regs *regs, long syscall_nr)
{
    unsigned long args[6];
    struct socket *sock = NULL;
    int fd;

    syscall_get_arguments(current, regs, args);


    switch (syscall_nr)
    {
    case __NR_socket:
        evt->net.protocol = args[2];
        evt->net_flag = 1;
        break;

    case __NR_connect:
    case __NR_accept:
        fd = args[0];
        sock = sockfd_lookup(fd, NULL);
        if (!sock)
            break;
        evt->net.direction = 1;     // 入站
        linx_get_socketcall_info(sock, &evt->net);
        evt->net_flag = 1;
        sockfd_put(sock);
        break;

    case __NR_bind:
    case __NR_listen:
        if (evt->type == SYSCALL_ENTER) {
            sock = sockfd_lookup(args[0], NULL);
            if (!sock)
                break;
            evt->net.direction = 1;     // 入站
            linx_get_socketcall_info(sock, &evt->net);
            evt->net_flag = 1;
            sockfd_put(sock);
        }
        break;

    case __NR_sendto:
    case __NR_sendmsg:
        fd = args[0];
        sock = sockfd_lookup(fd, NULL);
        if (!sock)
            break;
        evt->net.direction = 1;     // 入站
        linx_get_socketcall_info(sock, &evt->net);
        evt->net_flag = 1;
        sockfd_put(sock);
        break;
    
    // case __NR_recvfrom:
    // case __NR_recvmsg:
    //     fd = args[0];
    //     sock = sockfd_lookup(fd, NULL);
    //     if (!sock)
    //         break;
    //     evt->net.direction = 1;     // 入站
    //     struct msghdr msg;
    //     if (!copy_from_user(&msg, (void __user *)args[1], sizeof(msg))) {
    //         linx_get_socketcall_info(sock, &evt->net, msg.msg_iov, msg.msg_iov_len);
    //     }
    //     evt->net_flag = 1;
    //     sockfd_put(sock);
        // break;
    default:
        break;
    }

    
}

/**
 * @brief 系统调用入口探测点
 * 
 * @param __data 
 * @param regs 
 * @param id 
 */
static void sys_enter_probe(void *__data, struct pt_regs *regs, long id)
{
    if (id < 0)
        return;
    if (!linx_trace_syscall(id))
        return;

    // if (id != __NR_unlink && id != __NR_unlinkat)
    //     return;

    struct linx_kmod_consumer *consumer;
    struct task_struct *task = current;
    unsigned long args[6];

    event_header_t hdr;

    memset(&hdr, 0, sizeof(event_header_t));
    memset(args, 0, sizeof(args));

    // 填充事件头信息
    hdr.ts = ktime_get_real_ns();
    hdr.pid = current->tgid;
    hdr.tid = current->pid;
    hdr.syscall_nr = id;
    hdr.type = SYSCALL_ENTER;
    hdr.len = 0;
    hdr.res = 0;
    
    // 填充其他信息
    if (task->real_parent) {
        hdr.ppid = task->real_parent->pid;
        strncpy(hdr.pp_proc, task->real_parent->comm, sizeof(hdr.pp_proc)-1);
    }
    hdr.user = from_kuid(task->cred->user_ns, task->cred->uid);
    strncpy(hdr.proc, task->comm, sizeof(hdr.proc)-1);
    linx_get_cmdline(task, hdr.cmdline, sizeof(hdr.cmdline)-1);
    
    syscall_get_arguments(task, regs, args);

    if (linx_net_syscall(id)) {
        linx_net_info(&hdr, regs, id);
    }

    rcu_read_lock();
    list_for_each_entry_rcu(consumer, &linx_consumer_list, node) {
        linx_record_event(consumer, &hdr, args);
    }
    rcu_read_unlock();
}

/**
 * @brief 系统调用退出探针点
 * 
 * @param __data 
 * @param regs 
 * @param ret 
 */
static void sys_exit_probe(void *__data, struct pt_regs *regs, long ret)
{
    long id = syscall_get_nr(current, regs);

    if (id < 0)
        return;
    if (!linx_trace_syscall(id))
        return;

    struct linx_kmod_consumer *consumer;

    struct task_struct *task = current;
    unsigned long args[6];

    event_header_t hdr;

    memset(&hdr, 0, sizeof(event_header_t));

    // 填充事件头信息
    hdr.ts = ktime_get_real_ns();
    hdr.pid = current->tgid;
    hdr.tid = current->pid;
    hdr.syscall_nr = id;
    hdr.type = SYSCALL_EXIT;
    hdr.len = 0;
    hdr.res = ret;
    
    // 填充其他信息
    if (task->real_parent) {
        hdr.ppid = task->real_parent->pid;
        strncpy(hdr.pp_proc, task->real_parent->comm, sizeof(hdr.pp_proc)-1);
    }
    hdr.user = from_kuid(task->cred->user_ns, task->cred->uid);
    strncpy(hdr.proc, task->comm, sizeof(hdr.proc)-1);
    linx_get_cmdline(task, hdr.cmdline, sizeof(hdr.cmdline)-1);

    syscall_get_arguments(task, regs, args);
    
    rcu_read_lock();
    list_for_each_entry_rcu(consumer, &linx_consumer_list, node) {
        linx_record_event(consumer, &hdr, args);
    }
    rcu_read_unlock();
}


/**
 * @brief 任务切换探针点
 * 
 * @param __data 
 * @param preempt 
 * @param prev 
 * @param next 
 */
static void sched_switch_probe(void *__data, bool preempt,
                        struct task_struct *prev,
                        struct task_struct *next)
{
    // printk("sched_switch prev=%s next=%s \n", prev->comm, next->comm);
}


static struct tracepoint_entry {
    const char *name;
    struct tracepoint *tp;
    void *func;
}tracepoint[] = {
    {.name = "sys_enter", .func = sys_enter_probe},
    {.name = "sys_exit", .func = sys_exit_probe},
    {.name = "sched_switch", .func = sched_switch_probe},
    {}
};

/**
 * @brief 探针绑定
 * 
 * @param tp 
 * @param priv 
 */
static void visit_tracepoint(struct tracepoint *tp, void *priv) {
	if(!strcmp(tp->name, "sys_enter"))
		tracepoint[0].tp = tp;
	else if(!strcmp(tp->name, "sys_exit"))
		tracepoint[1].tp = tp;
    else if(!strcmp(tp->name, "sched_switch"))
		tracepoint[2].tp = tp;
}

/**
 * @brief 注册探针点
 * 
 */
static void compat_set_tracepoint(void)
{
    struct tracepoint_entry *iter;

    for (iter = tracepoint; iter->name != NULL; iter++)
    {
        tracepoint_probe_register(iter->tp, iter->func, NULL);
    }
    
}


/**
 * @brief 清理探针点
 * 
 */
static void clear_tracepoint(void)
{
    struct tracepoint_entry *iter;

    for (iter = tracepoint; iter->name != NULL; iter++)
    {
        tracepoint_probe_unregister(iter->tp, iter->func, NULL);
    }
}

/**
 * @brief 注册字符设备
 * 
 * @return int 
 */
 static int linx_register_cdev(void)
{
    dev_t dev;
    // unsigned int cpu;
    unsigned int num_cpus = 1;
    struct device *device = NULL;
    int ret;
    int j;
	int acrret = 0;
    int n_created_devices = 0;

    /* 获取CPU数量 */
    // num_cpus = 0;
	// for_each_possible_cpu(cpu) {
	// 	++num_cpus;
	// }

    g_kmod_numdevs = num_cpus;

    acrret = alloc_chrdev_region(&dev, 0, 1, DRIVER_DEVICE_NAME);
	if(acrret < 0) {
		pr_err("could not allocate major number for %s\n", DRIVER_DEVICE_NAME);
		ret = -ENOMEM;
		goto init_module_err;
	}

    g_kmod_class = class_create((const char *)CLASS_NAME);
    if(IS_ERR(g_kmod_class)) {
		pr_err("can't allocate device class\n");
		ret = -EFAULT;
		goto init_module_err;
	}

    g_kmod_major = MAJOR(dev);

    g_kmod_devs = kmalloc_array(g_kmod_numdevs, sizeof(struct linx_kmod_device), GFP_KERNEL);
    if(!g_kmod_devs) {
		pr_err("can't allocate devices\n");
		ret = -ENOMEM;
		goto init_module_err;
	}

    for(j = 0; j < g_kmod_numdevs; ++j) {
		cdev_init(&g_kmod_devs[j].cdev, &g_linx_kmod_fops);
		g_kmod_devs[j].dev = MKDEV(g_kmod_major, j);

		if(cdev_add(&g_kmod_devs[j].cdev, g_kmod_devs[j].dev, 1) < 0) {
			pr_err("could not allocate chrdev for %s\n", DRIVER_DEVICE_NAME);
			ret = -EFAULT;
			goto init_module_err;
		}

        device = device_create(g_kmod_class,
		        NULL, /* no parent device */
		        g_kmod_devs[j].dev,
		        NULL, /* no additional data */
		        DRIVER_DEVICE_NAME "%d",
		        j);
        if(IS_ERR(device)) {
			pr_err("error creating the device for  %s\n", DRIVER_DEVICE_NAME);
			cdev_del(&g_kmod_devs[j].cdev);
			ret = -EFAULT;
			goto init_module_err;
		}
        init_waitqueue_head(&g_kmod_devs[j].read_queue);
		n_created_devices++;
	}

    return 0;

init_module_err:
	for(j = 0; j < n_created_devices; ++j) {
		device_destroy(g_kmod_class, g_kmod_devs[j].dev);
		cdev_del(&g_kmod_devs[j].cdev);
	}

	if(g_kmod_class)
		class_destroy(g_kmod_class);

	if(acrret == 0)
		unregister_chrdev_region(dev, g_kmod_numdevs);

	kfree(g_kmod_devs);

	return ret;
}


/**
 * @brief 注销字符设备
 * 
 */
void linx_unregister_cdev(void)
{
	int j;

	pr_info("driver unloading\n");


	for(j = 0; j < g_kmod_numdevs; ++j) {
		device_destroy(g_kmod_class, g_kmod_devs[j].dev);
		cdev_del(&g_kmod_devs[j].cdev);
	}

	if(g_kmod_class)
		class_destroy(g_kmod_class);

	unregister_chrdev_region(MKDEV(g_kmod_major, 0), g_kmod_numdevs + 1);

	kfree(g_kmod_devs);

	tracepoint_synchronize_unregister();
}

/**
 * @brief 模块初始化
 * 
 * @return int 
 */
static int __init linx_init(void)
{
    pr_info("linx init\n");

    for_each_kernel_tracepoint(visit_tracepoint, NULL);
    compat_set_tracepoint();
    if (linx_register_cdev())
        pr_info("linx_register_cdev error\n");

    return 0;
}

/**
 * @brief 模块注销
 * 
 */
static void __exit linx_exit(void)
{
    pr_info("linx exit\n");
    clear_tracepoint();
    linx_unregister_cdev();
}

MODULE_LICENSE("GPL");
module_init(linx_init);
module_exit(linx_exit);

