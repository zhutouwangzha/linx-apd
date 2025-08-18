#ifndef __STRUCT_DEFINE_H__
#define __STRUCT_DEFINE_H__

#include "linx_size_define.h"

typedef struct {
    uint64_t boot_time;         /* 应用层获取到的启动时间，该时间+bpf中获取的时间=系统时间 */
    uint32_t snaplen;           /* 在特定系统调用中采集数据的长度 */
    bool drop_mode;             /* 丢弃模式的总体控制开关，为1时，放弃采集所有的系统调用 */
    bool drop_failed;           /* 是否放弃采集失败的系统调用为1时，放弃采集 */
    bool do_snaplen;            /* 是否进行采集流量控制 */
    uint16_t port_range_start;
    uint16_t port_range_stop;
} linx_capture_set_t;

typedef struct {
    uint8_t data[LINX_EVENT_MAX_SIZE * 2];
    uint8_t index;
    uint64_t payload_pos;
    uint64_t reserved_event_size;
} linx_ringbuf_t;

#endif /* __STRUCT_DEFINE_H__ */
