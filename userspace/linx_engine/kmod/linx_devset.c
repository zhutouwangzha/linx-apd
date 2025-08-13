#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdlib.h>

#include "linx_devset.h"
#include "linx_events_public.h"
#include "linx_engine_vtable.h"

#define CDEV_NAME   "/dev/clinx0"

struct kmod_device *dev_set = NULL;

#define BUFFER_SIZE (8 * 1024 * 1024)

/********************************************************************************
 * 内核模块驱动
***************************************************************************/
static int kmod_init(void)
{
    int dev_fd;

    dev_fd = open(CDEV_NAME, O_RDWR | O_SYNC);
    if (dev_fd < 0) {
        perror("open error\n");
        return -1;
    }

    dev_set = (struct kmod_device *)malloc(sizeof(struct kmod_device));
    if (!dev_set)
        return -1;

    dev_set->msg_fd = dev_fd;
    dev_set->msg_buffer = mmap(NULL, 2 * BUFFER_SIZE, PROT_READ, MAP_SHARED, dev_fd, 0);
    if (dev_set->msg_buffer == MAP_FAILED) {
        perror("mmap msg_buffer ");
        close(dev_fd);
        return -1;
    }

    dev_set->msg_buffer_size = sizeof(linx_event_t) + 6 * sizeof(uint64_t);
    dev_set->msg_mmap_size = BUFFER_SIZE;
    dev_set->msg_next_event = dev_set->msg_buffer;
    dev_set->log_fd = -1;

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

// static int kmod_config(int setting, unsigned long *args)
// {
//     if (ioctl(dev_set->msg_fd, setting, args) < 0) {
//         perror("ioctl ");
//         return -1;
//     }

//     return 0;
// }


static int kmod_next(linx_event_t **event)
{
    struct kmod_ringbuffer_info *ringinfo = dev_set->msg_bufinfo;
    linx_event_t *evt;
    uint32_t event_size;

    if (ringinfo->head != ringinfo->tail) {
        evt = (linx_event_t *)dev_set->msg_next_event;
        if (evt->size == 0)
            return -1;

        event_size = evt->size;
    } else {
        return -1;
    }

    ringinfo->tail += event_size;
    if (ringinfo->tail > BUFFER_SIZE) {
        ringinfo->tail -= BUFFER_SIZE;
        dev_set->msg_next_event -= BUFFER_SIZE;
    }

    dev_set->msg_next_event += event_size;
    *event = (linx_event_t *)evt;

    return 1;
}

static int kmod_close(void)
{
    char *buf = dev_set->msg_buffer;
    
    // 释放映射及关闭字符设备文件描述符
    munmap(buf, dev_set->msg_mmap_size);
    munmap(dev_set->msg_bufinfo, dev_set->msg_bufinfo_size);
    close(dev_set->msg_fd);
    free(dev_set);
    return 0;
}


linx_engine_vtable_t kmod_vtable = {
    .name = "kmod",
    .init = kmod_init,
    .start = NULL,
    .stop = NULL,
    .next = kmod_next,
    .close = kmod_close
};
