#ifndef __LINX_EVENT_RICH_H__
#define __LINX_EVENT_RICH_H__

#include <stdint.h>

#include "linx_event.h"
#include "event.h"

typedef struct {
    uint8_t family;
    union {
        struct {
            uint32_t ip1;
            uint16_t port1;
            uint32_t ip2;
            uint16_t port2;
        } ipv4;

        struct {
            uint32_t ip1[4];
            uint16_t port1;
            uint32_t ip2[4];
            uint16_t port2;
        } ipv6;

        struct {
            uint64_t sock[2];
            char path[0];
        } af_unix;
    } data;
} socktuple_t;

int linx_event_rich_init(void);

void linx_event_rich_deinit(void);

int linx_event_rich(linx_event_t *event);

event_t *linx_event_rich_get(void);

/* 多线程支持函数 */
int linx_event_rich_bind_field(void);

void rich_event_clean(linx_event_type_t type);

#endif /* __LINX_EVENT_RICH_H__ */
