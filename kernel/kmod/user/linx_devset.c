#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "user/linx_devset.h"
#include "kmod_msg.h"
#include "user/linx_logs.h"

#define CDEV_NAME   "/dev/clinx0"

#define BUFFER_SIZE     (32 * 1024)   // 每个CPU 8MB缓冲区

static int linx_kmod_init(struct kmod_device *dev_set, char *filename)
{
    int dev_fd;
    int logs_fd;

    dev_fd = open(CDEV_NAME, O_RDWR | O_SYNC);
    if (dev_fd < 0) {
        perror("open error\n");
        return -1;
    }

    logs_fd = linx_logs_open(filename);
    if (logs_fd < 0) {
        close(dev_fd);
        return -1;
    }


    dev_set->msg_fd = dev_fd;
    dev_set->msg_buffer = mmap(NULL, 2 * 32 * 1024, PROT_READ, MAP_SHARED, dev_fd, 0);
    if (dev_set->msg_buffer == MAP_FAILED) {
        perror("mmap msg_buffer ");
        close(dev_fd);
        return -1;
    }

    dev_set->msg_buffer_size = sizeof(event_header_t) + 6 * sizeof(uint64_t);
    dev_set->msg_mmap_size = 2 * 32 * 1024;
    dev_set->msg_next_event = dev_set->msg_buffer;
    dev_set->log_fd = logs_fd;

    dev_set->msg_bufinfo = mmap(NULL, sizeof(struct kmod_ringbuffer_info), PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if (dev_set->msg_bufinfo == MAP_FAILED) {
        perror("mmap msg_bufinfo ");
        munmap(dev_set->msg_buffer, dev_set->msg_mmap_size);
        close(dev_fd);
        return -1;
    }

    dev_set->msg_bufinfo_size = sizeof(struct kmod_ringbuffer_info);
    return 0;
}

static int linx_kmod_config(struct kmod_device *dev_set, int setting, unsigned long *args)
{
    if (ioctl(dev_set->msg_fd, setting, args) < 0) {
        perror("ioctl ");
        return -1;
    }

    return 0;
}

static int linx_kmod_next(struct kmod_device *dev_set)
{
    struct kmod_ringbuffer_info *ringinfo = dev_set->msg_bufinfo;
    event_header_t *evt;
    uint32_t event_size;

    if (ringinfo->head != ringinfo->tail) {
        evt = (event_header_t *)dev_set->msg_next_event;
        if (evt->len == 0)
            return -1;

        linx_logs_write(dev_set->log_fd, evt);
        linx_log_print(evt);
        printf("evt_len = %d\n", evt->len);
        event_size = evt->len;
    } else {
        return -1;
    }

    // event_size = sizeof(event_header_t);
    ringinfo->tail += event_size;
    if (ringinfo->tail > BUFFER_SIZE)
        ringinfo->tail -= BUFFER_SIZE;
    dev_set->msg_next_event += event_size;
    printf("tail = %x\n", ringinfo->tail);
    
    return 0;
}

static int linx_kmod_close(struct kmod_device *dev_set)
{
    char *buf = dev_set->msg_buffer;

    // 关闭日志文件描述
    if (dev_set->log_fd > 0)
        linx_logs_close(dev_set->log_fd);
    
    // 释放映射及关闭字符设备文件描述符
    munmap(buf, dev_set->msg_mmap_size);
    munmap(dev_set->msg_bufinfo, dev_set->msg_bufinfo_size);
    close(dev_set->msg_fd);
    return 0;
}


struct kmod_ops linx_kmod_engine = {
    .kmod_init = linx_kmod_init,
    .kmod_config = linx_kmod_config,
    .komd_next = linx_kmod_next,
    .komd_close = linx_kmod_close,
};