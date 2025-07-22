#include <time.h>

#include "linx_event_rich.h"
#include "linx_hash_map.h"
#include "linx_log.h"
#include "event.h"

#include "linx_syscall_table.h"

static event_t evt = {0};

int linx_event_rich_init(void)
{
    /* 初始化 evt 结构体成员映射 */
    int ret = linx_hash_map_init();
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_init failed");
        return -1;
    }

    ret = linx_hash_map_create_table("evt", &evt);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_create_table failed");
        linx_hash_map_deinit();
        return -1;
    }

    ret = linx_hash_map_add_field("evt", "num", offsetof(event_t, num), sizeof(evt.num), FIELD_TYPE_UINT64);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }
    
    ret = linx_hash_map_add_field("evt", "time", offsetof(event_t, time), sizeof(evt.time), FIELD_TYPE_CHARBUF);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }
    
    ret = linx_hash_map_add_field("evt", "type", offsetof(event_t, type), sizeof(evt.type), FILED_TYPE_CHARBUF_ARRAY);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }
    
    ret = linx_hash_map_add_field("evt", "args", offsetof(event_t, args), sizeof(evt.args), FIELD_TYPE_CHARBUF);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }
    
    ret = linx_hash_map_add_field("evt", "res", offsetof(event_t, res), sizeof(evt.res), FIELD_TYPE_CHARBUF);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }

    ret = linx_hash_map_add_field("evt", "rawres", offsetof(event_t, rawres), sizeof(evt.rawres), FIELD_TYPE_CHARBUF);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }    

    ret = linx_hash_map_add_field("evt", "failed", offsetof(event_t, failed), sizeof(evt.failed), FIELD_TYPE_BOOL);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }

    ret = linx_hash_map_add_field("evt", "dir", offsetof(event_t, dir), sizeof(evt.dir), FIELD_TYPE_CHARBUF);
    if (ret) {
        LINX_LOG_WARNING("add field name offset failed");
    }

    return 0;
}

int linx_event_rich(linx_event_t *event)
{
    /**
     * 根据event事件类型
     * 判断是否有改变工作目录，改变用户等操作
     * 同步更新到应用层保存的结构体中
    */

    /* 更新 evt 结构体相关内容 */
    uint64_t ns = event->time;
    uint64_t remaining_ns = ns % 1000000000;
    time_t seconds = ns / 1000000000;
    struct tm *timeinfo = localtime(&seconds);
    size_t len = strftime(evt.time, sizeof(evt.time), "%Y-%m-%d %H:%M:%S", timeinfo);

    snprintf(evt.time + len, sizeof(evt.time) - len, ".%09lu", remaining_ns);

    evt.num = event->syscall_id;
    event->type == LINX_SYSCALL_TYPE_ENTER ? 
        strcpy(evt.dir, ">") : 
        strcpy(evt.dir, "<");
    evt.type = g_linx_syscall_table[event->syscall_id].name;
    evt.rawres = (int64_t)event->res;
    if (evt.rawres == 0) {
        evt.failed = false;
        strcpy(evt.res, "SUCCESS");
    } else {
        evt.failed = true;
        strcpy(evt.res, "ERRNO");
    }

    return 0;
}

int linx_event_rich_deinit(void)
{
    return 0;
}