#include "event.h"
#include "linx_log.h"
#include "linx_event_queue.h"
#include "linx_hash_map.h"

event_t evt = {
    .cpu = 0,
    .dir = {'<'},
    .num = 1,
    .time = 13132,
    .type = 2
};

int linx_event_queue_init(uint64_t capacity)
{
    (void)capacity;

    int ret;

    ret = linx_hash_map_init();
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_init failed");
    }

    ret = linx_hash_map_create_table("evt", &evt);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_create_table failed");
    }

    linx_hash_map_add_field("evt", "cpu", offsetof(event_t, cpu), sizeof(evt.cpu), FIELD_TYPE_INT64);
    linx_hash_map_add_field("evt", "dir", offsetof(event_t, dir), sizeof(evt.dir), FIELD_TYPE_STRING);
    linx_hash_map_add_field("evt", "num", offsetof(event_t, num), sizeof(evt.num), FIELD_TYPE_INT64);
    linx_hash_map_add_field("evt", "time", offsetof(event_t, time), sizeof(evt.time), FIELD_TYPE_INT64);
    linx_hash_map_add_field("evt", "type", offsetof(event_t, type), sizeof(evt.type), FIELD_TYPE_INT64);

    return 0;
}

int linx_event_queue_push(void)
{
    return 0;
}

void linx_event_queue_free(void)
{

}
