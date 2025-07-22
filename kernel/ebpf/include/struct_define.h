#ifndef __STRUCT_DEFINE_H__
#define __STRUCT_DEFINE_H__

#include "linx_size_define.h"

typedef struct {
    uint8_t data[LINX_EVENT_MAX_SIZE * 2];
    uint8_t index;
    uint64_t payload_pos;
    uint64_t reserved_event_size;
} linx_ringbuf_t;

#endif /* __STRUCT_DEFINE_H__ */
