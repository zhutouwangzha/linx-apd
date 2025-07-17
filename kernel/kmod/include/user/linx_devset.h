#ifndef __LINX_DEVSET_H__
#define __LINX_DEVSET_H__

#include <stdint.h>

typedef struct kmod_device
{
    int msg_fd;                     // 字符设备文件描述符
    char *msg_buffer;               // 消息缓冲区
    unsigned long msg_buffer_size;  // 消息大小
    unsigned long msg_mmap_size;    // 映射区大小
    char* msg_next_event;           // 下一事件
    int log_fd;                     // 日志文件描述符
    struct kmod_ringbuffer_info *msg_bufinfo;   // 消息结构
    int msg_bufinfo_size;           // 消息结构大小
}kmod_device_t;


typedef struct kmod_ops
{
    int (*kmod_init) (struct kmod_device *dev_set, char *filename);
    int (*kmod_config) (struct kmod_device *dev_set, int setting, unsigned long *args);
    int (*komd_next) (struct kmod_device *dev_set);
    int (*komd_close) (struct kmod_device *dev_set);
}kmod_ops_t;

#endif /*__LINX_DEVSET_H__*/