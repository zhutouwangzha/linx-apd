#ifndef __LINX_CONSUMER_H__
#define __LINX_CONSUMER_H__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>


#define MAX_SYSCALLS_NUM   (512)

typedef struct kmod_consumer_config
{
    uint8_t cap_enable;        /* 捕获使能 */
    uint8_t filter_enable;     /* 过滤功能使能  */
    uint8_t args_enable;       /* 参数输出使能 */
    uint8_t fds_enabel;        /* 进程文件描述符集使能 */
    uint8_t drop_enabel;       /* 丢弃模式 */
}kmod_consumer_config_t;

struct linx_kmod_consumer
{
    unsigned int id;                    // 消费者id
    struct list_head node;              // 链表节点
    struct task_struct *consumer_id;    // 消费者进程
    struct kmod_ringbuffer_ctx *ring;   // 环形缓冲区
    struct mutex lock;                  // 互斥锁
    struct kmod_consumer_config consumer_cfg;   // 消费者配置
    unsigned long syscall_mask[MAX_SYSCALLS_NUM/ __BITS_PER_LONG]; //系统调用位图
};

#endif /* __LINX_CONSUMER_H__ */