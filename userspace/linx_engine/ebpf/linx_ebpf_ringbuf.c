#include <string.h>

#include "linx_ebpf_common.h"
#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_event.h"

static uint8_t *g_ringbuf_data;

static int linx_handle_event(void *ctx, void *data, size_t data_sz)
{
    (void)ctx;

    if (data_sz > LINX_EVENT_MAX_SIZE) {
        LINX_LOG_WARNING("The data length of %lu get from ringbuf exceeds the limit of %lu!",
                         data_sz, LINX_EVENT_MAX_SIZE);
        return -1;
    }

    memset(g_ringbuf_data, 0, LINX_EVENT_MAX_SIZE);
    memcpy(g_ringbuf_data, data, data_sz);

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

    g_ringbuf_data = &bpf_manager->ringbuf_data[0];

    return 0;
}

int linx_ebpf_get_ringbuf_msg(linx_ebpf_t *bpf_manager, linx_event_t **event)
{
    int ret = ring_buffer__poll(bpf_manager->rb, 0);
    if (ret < 0) {
        /* 说明失败了 */
        *event = NULL;
    } else if (ret == 0) {
        /* 说明没有数据 */
        *event = NULL;
    } else {
        *event = (linx_event_t *)g_ringbuf_data;
    }

    return ret;
}
