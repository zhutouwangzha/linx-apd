#ifndef __MAPS_GET_H__
#define __MAPS_GET_H__

#include "maps.h"
#include "bpf_common.h"

static inline linx_capture_set_t *maps_get_capture_set(void)
{
    uint32_t key = 0;
    return bpf_map_lookup_elem(&g_capture_set, &key);
}

static inline uint64_t maps_get_boot_time(void)
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return 0;
    }

    return set->boot_time;
}

static inline bool maps_get_drop_mode(void)
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return false;
    }

    return set->drop_mode;
}

static inline bool maps_get_drop_failed(void)
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return false;
    }

    return set->drop_failed;
}

static inline uint32_t maps_get_snaplen(void)
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return 0;
    }

    return set->snaplen > LINX_SNAPLEN_MAX ? LINX_SNAPLEN_MAX : set->snaplen;
}

static inline bool maps_get_do_snaplen(void)
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return false;
    }

    return set->do_snaplen;
}

static inline uint16_t maps_get_port_range_start()
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return 0;
    }

    return set->port_range_start;
}

static inline uint16_t maps_get_port_range_end()
{
    linx_capture_set_t *set = maps_get_capture_set();
    if (set == NULL) {
        return 0;
    }

    return set->port_range_stop;
}

#endif /* __MAPS_GET_H__ */
