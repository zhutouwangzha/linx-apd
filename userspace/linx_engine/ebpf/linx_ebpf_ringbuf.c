#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "linx_ebpf_common.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_event.h"

/*
 * Userspace ring buffer to queue events drained from libbpf ring buffer.
 * This prevents overwriting a single static buffer when multiple events
 * arrive in one poll cycle and avoids dropping events at high rates.
 */
#ifndef LINX_EBPF_USER_RING_CAP
#define LINX_EBPF_USER_RING_CAP 16384
#endif

typedef struct linx_user_evt_slot_s {
    size_t sz;
    uint8_t data[LINX_EVENT_MAX_SIZE];
} linx_user_evt_slot_t;

static linx_user_evt_slot_t *g_user_ring = NULL;
static uint32_t g_user_ring_cap = LINX_EBPF_USER_RING_CAP;
static atomic_uint g_user_ring_head;
static atomic_uint g_user_ring_tail;

static inline uint32_t linx_user_ring_next(uint32_t idx) {
    return (uint32_t)((idx + 1) % g_user_ring_cap);
}

static inline int linx_user_ring_is_full(uint32_t head, uint32_t tail) {
    return linx_user_ring_next(tail) == head;
}

static inline int linx_user_ring_is_empty(uint32_t head, uint32_t tail) {
    return head == tail;
}

static int linx_user_ring_push(const void *data, size_t data_sz)
{
    uint32_t head = atomic_load_explicit(&g_user_ring_head, memory_order_acquire);
    uint32_t tail = atomic_load_explicit(&g_user_ring_tail, memory_order_relaxed);

    if (!g_user_ring || data_sz > LINX_EVENT_MAX_SIZE) {
        return -1;
    }

    if (linx_user_ring_is_full(head, tail)) {
        /* Drop oldest: advance head to make room */
        atomic_store_explicit(&g_user_ring_head, linx_user_ring_next(head), memory_order_release);
        head = atomic_load_explicit(&g_user_ring_head, memory_order_acquire);
    }

    uint32_t next_tail = linx_user_ring_next(tail);
    g_user_ring[tail].sz = data_sz;
    memcpy(g_user_ring[tail].data, data, data_sz);
    atomic_store_explicit(&g_user_ring_tail, next_tail, memory_order_release);

    return 0;
}

static int linx_user_ring_pop(linx_event_t **event)
{
    uint32_t head = atomic_load_explicit(&g_user_ring_head, memory_order_acquire);
    uint32_t tail = atomic_load_explicit(&g_user_ring_tail, memory_order_acquire);

    if (!g_user_ring || linx_user_ring_is_empty(head, tail)) {
        *event = NULL;
        return 0;
    }

    *event = (linx_event_t *)g_user_ring[head].data;
    atomic_store_explicit(&g_user_ring_head, linx_user_ring_next(head), memory_order_release);
    return 1;
}

static int linx_handle_event(void *ctx, void *data, size_t data_sz)
{
    (void)ctx;

    if (data_sz > LINX_EVENT_MAX_SIZE) {
        LINX_LOG_WARNING("The data length of %lu get from ringbuf exceeds the limit of %lu!",
                         data_sz, LINX_EVENT_MAX_SIZE);
        return -1;
    }

    if (linx_user_ring_push(data, data_sz)) {
        return -1;
    }

    return 0;
}

int linx_ebpf_ringbuf_init(linx_ebpf_t *bpf_manager)
{
    bpf_manager->rb = 
        ring_buffer__new(bpf_map__fd(bpf_manager->skel->maps.ringbuf_map),
                         linx_handle_event, NULL, NULL);
    if (!bpf_manager->rb) {
        LINX_LOG_ERROR("Failed to create ringbuf!");
        ring_buffer__free(bpf_manager->rb);
        linx_bpf__destroy(bpf_manager->skel);
        return -1;
    }

    /* allocate userspace ring */
    if (!g_user_ring) {
        g_user_ring = (linx_user_evt_slot_t *)calloc(g_user_ring_cap, sizeof(linx_user_evt_slot_t));
        if (!g_user_ring) {
            LINX_LOG_ERROR("Failed to allocate userspace ring buffer");
            ring_buffer__free(bpf_manager->rb);
            linx_bpf__destroy(bpf_manager->skel);
            return -1;
        }
        atomic_store_explicit(&g_user_ring_head, 0, memory_order_relaxed);
        atomic_store_explicit(&g_user_ring_tail, 0, memory_order_relaxed);
    }

    return 0;
}

int linx_ebpf_get_ringbuf_msg(linx_ebpf_t *bpf_manager, linx_event_t **event)
{
    int ret;

    ret = linx_user_ring_pop(event);
    if (ret > 0) {
        return ret;
    }

    /* Drain kernel ringbuf; timeout 0 for non-blocking, then try pop again */
    ret = ring_buffer__poll(bpf_manager->rb, 0);
    if (ret < 0) {
        *event = NULL;
        return ret;
    }

    ret = linx_user_ring_pop(event);
    if (ret > 0) {
        return ret;
    }

    *event = NULL;
    return 0;
}
