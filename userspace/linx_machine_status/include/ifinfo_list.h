#ifndef __IFINFO_LIST_H__
#define __IFINFO_LIST_H__

#include <stdint.h>

typedef struct {
    char net_mask[4];
    char ifname[1024];
} ifinfo_ipv4_t;

typedef struct {
    char net_mask[4];
    char ifname[1024];
} ifinfo_ipv6_t;

typedef struct {
    uint32_t n_v4;
    uint32_t n_v6;
    ifinfo_ipv4_t *v4list;
    ifinfo_ipv6_t *v6list;
} ifinfo_list_t;

#endif /* __IFINFO_LIST_H__ */
