#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <asm/syscall.h>
#include <linux/init.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/atmioc.h>
#include <linux/list.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <asm/unistd.h>
#include <asm/page.h>
#include <linux/errno.h>
#include <linux/mm.h>

#include "linx_events_public.h"
#include "linx_consumer.h"
#include "linx_events.h"
#include "linx_event.h"
#include "linx_event_type.h"
#include "linx_field_type.h"
#include "linx_event_table.h"


#define DRIVER_DEVICE_NAME  "clinx"
#define CLASS_NAME   "my_class"
#define SYSMON_SYSCALL_MAX (LINX_EVENT_TYPE_MAX / 2)

/******************************************************************
 * 
******************************************************************/
struct kmod_ringbuffer_ctx
{
    struct kmod_ringbuffer_info *ring_info;
    char *buffer;           // 缓冲区数据指针
    uint32_t size;          // 缓冲区大小
    char *str_storage;
    atomic_t preempt_count; // 抢占计数器
    bool open;              // 打开标志
};

// 设备结构体
struct linx_kmod_device {
	dev_t dev;              // 设备号
	struct cdev cdev;       // 字符设备
	wait_queue_head_t read_queue;
};

/******************************************************************
 * 
******************************************************************/

// 文件操作集
static int linx_kmod_open(struct inode *inode, struct file *filp);
static int linx_kmod_release(struct inode *inode, struct file *filp);
static long linx_kmod_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int linx_kmod_mmap(struct file *filp, struct vm_area_struct *vma);

static int linx_init_ringbuffer(struct kmod_ringbuffer_ctx *ring, unsigned long ringlen);
static void linx_free_ringbuffer(struct kmod_ringbuffer_ctx *ring);

/******************************************************************
 * 
******************************************************************/
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


/********************************************************************************
 * 调试接口
***************************************************************************/
void linx_print_args(char *buf, linx_event_type_t event_type, uint64_t *pt_len)
{
    uint32_t n_args = 0;
    uint32_t pt_type = -1;
    uint32_t pt_offset = 0;
    uint32_t pt_print_fmt = -1;
    
    if (buf == NULL || event_type < 0 || event_type > SYSMON_SYSCALL_MAX) {
        printk("buf is NULL or event_type < 0\n");        
        return;
    }

    n_args = g_linx_event_table[event_type].nparams;
    if (n_args > 32)
        return;

    for (int i = 0; i < n_args; i++)
    {
        pt_type = g_linx_event_table[event_type].params[i].type;
        pt_print_fmt = PF_DEC;

        switch (pt_type)
        {
        case LINX_FIELD_TYPE_CHARBUF:
            printk("%d_args = %s \n", i, (char *)(buf + pt_offset));
            break;
        case LINX_FIELD_TYPE_INT32:
            if (pt_print_fmt == PF_OCT)
                printk("%d_args = %o(oct) \n", i, *((int32_t *)(buf + pt_offset)));
            else if (pt_print_fmt == PF_HEX)
                printk("%d_args = %#x \n", i, *((int32_t *)(buf + pt_offset)));
            else
                printk("%d_args = %d \n", i, *((int32_t *)(buf + pt_offset)));
            
            break;
        case LINX_FIELD_TYPE_UINT32:
            if (pt_print_fmt == PF_OCT)
                printk("%d_args = %o(oct) \n", i, *((uint32_t *)(buf + pt_offset)));
            else if (pt_print_fmt == PF_HEX)
                printk("%d_args = %#x \n", i, *((uint32_t *)(buf + pt_offset)));
            else
                printk("%d_args = %u \n", i, *((uint32_t *)(buf + pt_offset)));
            
            break;
        case LINX_FIELD_TYPE_INT64:
            if (pt_print_fmt == PF_OCT)
                printk("%d_args = %o(oct) \n", i, *((int64_t *)(buf + pt_offset)));
            else if (pt_print_fmt == PF_HEX)
                printk("%d_args = %#x \n", i, *((int64_t *)(buf + pt_offset)));
            else
                printk("%d_args = %lld \n", i, *((int64_t *)(buf + pt_offset)));
            
            break;
        case LINX_FIELD_TYPE_UINT64:
            if (pt_print_fmt == PF_OCT)
                printk("%d_args = %o(oct) \n", i, *((uint64_t *)(buf + pt_offset)));
            else if (pt_print_fmt == PF_HEX)
                printk("%d_args = %#x \n", i, *((uint64_t *)(buf + pt_offset)));
            else
                printk("%d_args = %llu \n", i, *((uint64_t *)(buf + pt_offset)));
            
            break;
        case LINX_FIELD_TYPE_BYTEBUF:
            printk("%d_args = %llu \n", i, *((uint64_t *)(buf + pt_offset)));
            // printk("%d_args = %s \n", i, ((char *)(buf + pt_offset)));
            break;
#if 0
        case PT_SOCKTUPLE:
            {
                char *tmp_ptr = buf + pt_offset;
                uint32_t family = *((uint16_t *)(tmp_ptr));
                uint32_t sip_offset = 0;
                uint16_t sport_offset = 0;
                uint32_t dip_offset = 0;
                uint16_t dport_offset = 0;
                
                printk("1)family = %#x\n", family);

                if (family == AF_INET) {
                    sip_offset = 1;
                    sport_offset = 5;
                    dip_offset = 7;
                    dport_offset = 11;
                } else if (family == AF_INET6) {
                    sip_offset = 1;
                    sport_offset = 17;
                    dip_offset = 19;
                    dport_offset = 35;
                } else {
                    break;
                }
                    
                printk("2)sip = %u.%u.%u.%u\n", tmp_ptr[sip_offset], tmp_ptr[sip_offset + 1], tmp_ptr[sip_offset + 2], tmp_ptr[sip_offset + 3]);
                printk("3)sport = %u \n", (uint16_t)tmp_ptr[sport_offset]);
                printk("4)dip = %u.%u.%u.%u\n", tmp_ptr[dip_offset], tmp_ptr[dip_offset + 1], tmp_ptr[dip_offset + 2], tmp_ptr[dip_offset + 3]);
                printk("5)dport = %u \n",  (uint16_t)tmp_ptr[dport_offset]);
                break;
            }
#endif
        default:
            break;
        }
        pt_offset += pt_len[i];
    }
    
}

void linx_evt_print(linx_event_t *evt)
{
    char *buf = kzalloc(sizeof(char) * 2048, GFP_KERNEL);
    if (!buf) {
        return;
    }

    snprintf(buf, 2048, "syscall=%u tid=%lld pid=%lld ppid=%lld uid=%lld gid=%lld res=%lld\n",
                evt->type, evt->tid, evt->pid, evt->ppid, evt->uid, evt->gid, evt->res);


    printk("%s", buf);
    kfree(buf);
    return;
}


/********************************************************************************
 * 内核模块驱动
***************************************************************************/
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
        return -ENODEV;
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
            ret = -ENOMEM;
            goto out;
        }

        consumer->consumer_id = current;
        mutex_init(&consumer->lock);
        list_add_rcu(&consumer->node, &linx_consumer_list);
    } else {
        if (consumer->ring->open) {
            ret = -EBUSY;
            goto out;
        }
    }

    consumer->ring->open = true;
    consumer->consumer_cfg.drop_enabel = DISABLE;
    consumer->consumer_cfg.fds_enabel = DISABLE;
    consumer->consumer_cfg.args_enable = ENABLE;
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
    int ret = 0;
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
    {
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
        
    }
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
    ring->str_storage = (char *)__get_free_page(GFP_USER);
    if(!ring->str_storage) {
		return -ENOMEM;
	}

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

/**
 * @brief 清理环形缓冲区资源
 * 
 * @param ring 
 */
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

    if (ring->str_storage) {
        free_page((unsigned long)ring->str_storage);
        ring->str_storage = NULL;
    }
    ring->size = 0;
}

/**
 * @brief 获取详细参数
 * 
 * @param args 
 * @param evt_type 
 * @param buf 
 * @param buflen 
 * @return int 
 */
// int linx_get_params(struct event_filler_arguments *args,
//                linx_event_type_t evt_type, char *buf, size_t buflen)
int linx_get_params(struct event_filler_arguments *args,
               linx_event_type_t evt_type)
{
    int ret;
    linx_event_type_t event_type = evt_type;

    args->nargs = g_linx_event_table[event_type].nparams;
    args->arg_data_offset = 0;
    // args->buffer = buf;
    // args->arg_data_size = buflen;
    args->syscall_nr = (evt_type / 2);

    args->curarg = 0;
    args->event_type = event_type;

    /* 获取实际参数 */
    if(likely(g_sysmon_events[event_type].filler_callback)) {
        ret = g_sysmon_events[event_type].filler_callback(args);
        return ret;
    } else {
        /* 不处理直接返回 */
        args->arg_data_offset = 0;
        args->arg_data_size = 0;
        return -1;
    }

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
static int linx_record_event(struct linx_kmod_consumer *consumer, linx_event_t *evt, struct pt_regs *regs)
{
    struct kmod_ringbuffer_ctx *ring = consumer->ring;
    linx_event_t *hdr;
    uint32_t event_size;
    uint32_t free_space;
    uint32_t next;
    uint32_t delta_from_end;
    struct event_filler_arguments real_args = {0};
    unsigned long args[6];
    int ret;

    // 判断系统调用退出操作
    if ((evt->type % 2) != 0) {
        ret = (int64_t)syscall_get_return_value(current, regs);
			if(ret < 0) {
				return 0;
			}
    }

    memset(&real_args, 0, sizeof(struct event_filler_arguments));
    
    // 计算可用空间
    if (ring->ring_info->tail > ring->ring_info->head)
        free_space = ring->ring_info->tail  - ring->ring_info->head - 1;
    else
        free_space = ring->size + ring->ring_info->tail - ring->ring_info->head - 1;

    // 剩余可用空间
    delta_from_end = consumer->ring->size + (2 * PAGE_SIZE) - ring->ring_info->head - 1;

    if (free_space > consumer->ring->size)
        return 0;

    if ((delta_from_end >= consumer->ring->size + (2 * PAGE_SIZE)) || 
        (delta_from_end < (2 * PAGE_SIZE) - 1))
        return 0;

    syscall_get_arguments(current, regs, args);

    // 获取抢占锁
    if (atomic_inc_return(&ring->preempt_count) != 1){
        atomic_dec(&ring->preempt_count);
        return 0;
    }

    /* 判断空间剩余 */
    if (free_space < sizeof(linx_event_t)) {
        atomic_dec(&ring->preempt_count);
        ring->ring_info->n_drop_evts++;
        evt->size = 0;
        return 0;
    }

    hdr = (linx_event_t *)(ring->buffer + ring->ring_info->head);

    /* 填充头部信息 */
    event_size = sizeof(linx_event_t);
    memcpy(hdr, evt, event_size);

    /* 获取详细参数 */
    if (consumer->consumer_cfg.args_enable) {
        memcpy(real_args.args, args, sizeof(real_args.args));
        real_args.consumer = consumer;
        real_args.regs = regs;
        real_args.str_storage = ring->str_storage;

        real_args.buffer = ring->buffer + ring->ring_info->head + sizeof(linx_event_t);
        real_args.arg_data_size = min(free_space, delta_from_end - sizeof(linx_event_t));

        ret = linx_get_params(&real_args, evt->type);
        if (ret < 0)
            event_size = sizeof(linx_event_t);
        else {
            event_size = sizeof(linx_event_t) + real_args.arg_data_offset;
            evt->nparams = real_args.nargs;
            memcpy(hdr->params_size, real_args.args_len, sizeof(int64_t) * evt->nparams);
        }
    }

    hdr->size = event_size;

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
    linx_evt_print(evt);

    atomic_dec(&ring->preempt_count);
    return 0;
}


/**
 * @brief 跟踪系统调用（测试使用）
 * 
 * @param id 
 * @return int 
 */
static int linx_trace_syscall(long id)
{
    switch (id)
    {
    // case __NR_unlink:
    // case __NR_unlinkat:
    // case __NR_open:
    // case __NR_openat:
    // case __NR_write:
    // case __NR_close:
    // case __NR_read:
    case __NR_execve:
    // case __NR_fork:
    // case __NR_socket:
    // case __NR_connect:
    // case __NR_bind:
    // case __NR_listen:
    // case __NR_sendto:
    // case __NR_sendmsg:
    // case __NR_recvfrom:
    // case __NR_recvmsg:
    // case __NR_dup:
    // case __NR_dup2:
    // case __NR_dup3:
        return 1;
        break;
    
    default:
        return 0;
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

    struct linx_kmod_consumer *consumer = NULL;
    struct task_struct *task = current;
    const struct cred *cred = task->cred;
    linx_event_t *hdr;

    hdr = vmalloc(sizeof(linx_event_t));
    if (!hdr)
        return;

    memset(hdr, 0, sizeof(linx_event_t));

    // 填充事件头信息
    hdr->time = ktime_get_real_ns();
    hdr->pid = current->tgid;
    hdr->tid = current->pid;
    hdr->type = id * 2 ;
    hdr->size = 0;
    hdr->res = 0;
    hdr->uid = cred->uid.val;
    hdr->gid = cred->gid.val;

    // 填充其他信息
    if (task->real_parent) {
        hdr->ppid = task->real_parent->pid;
    }

    strncpy(hdr->comm, task->comm, sizeof(hdr->comm)-1);

    rcu_read_lock();
    list_for_each_entry_rcu(consumer, &linx_consumer_list, node) {
        linx_record_event(consumer, hdr, regs);
    }
    rcu_read_unlock();
    vfree(hdr);
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
    linx_event_t *hdr;
    const struct cred *cred = task->cred;

    hdr = vmalloc(sizeof(linx_event_t));
    if (!hdr)
        return;

    memset(hdr, 0, sizeof(linx_event_t));

    // 填充头部信息
    hdr->time = ktime_get_real_ns();
    hdr->pid = current->tgid;
    hdr->tid = current->pid;
    hdr->type = id * 2 + 1;
    hdr->size = 0;
    hdr->res = ret;
    hdr->uid = cred->uid.val;
    hdr->gid = cred->gid.val;

    // 填充其他信息
    if (task->real_parent) {
        hdr->ppid = task->real_parent->pid;
    }
    
    strncpy(hdr->comm, task->comm, sizeof(hdr->comm)-1);

    rcu_read_lock();
    list_for_each_entry_rcu(consumer, &linx_consumer_list, node) {
        linx_record_event(consumer, hdr, regs);
    }
    rcu_read_unlock();
    vfree(hdr);
}


static struct tracepoint_entry {
    const char *name;
    struct tracepoint *tp;
    void *func;
}tracepoint[] = {
    {.name = "sys_enter", .func = sys_enter_probe},
    {.name = "sys_exit", .func = sys_exit_probe},
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
    unsigned int cpu;
    unsigned int num_cpus = 0;
    struct device *device = NULL;
    int ret;
    int j;
	int acrret = 0;
    int n_created_devices = 0;

    /* 获取CPU数量 */
    num_cpus = 0;
	for_each_possible_cpu(cpu) {
		++num_cpus;
	}

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
    pr_info("linx apd init\n");

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
    pr_info("linx apd exit\n");
    clear_tracepoint();
    linx_unregister_cdev();
}

MODULE_LICENSE("GPL");
module_init(linx_init);
module_exit(linx_exit);