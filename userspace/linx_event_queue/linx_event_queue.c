#include "event.h"
#include "linx_log.h"
#include "linx_event_queue.h"
#include "linx_hash_map.h"

#include "linx_event.h"
#include "linx_ebpf_api.h"
int linx_event_queue_init(uint64_t capacity)
{
    (void)capacity;

    return 0;
}

int linx_event_queue_push(void)
{
    return 0;
}

void linx_event_queue_free(void)
{

}
