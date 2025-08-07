#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

#include "linx_event_rich.h"
#include "linx_hash_map.h"
#include "linx_log.h"

#include "linx_event_table.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"

static event_t evt = {0};

static int update_field_base(pid_t pid)
{
    field_update_table_t tables[] = {
        {"evt", &evt},
        {"proc", (void *)linx_process_cache_get(pid)},
        {"user", (void *)linx_machine_status_get_user()},
        {"group", (void *)linx_machine_status_get_group()},
    };

    return linx_hash_map_update_tables_base(tables, sizeof(tables) / sizeof(tables[0]));
}

static int linx_event_rich_bind_field(void)
{
    int ret;

    BEGIN_FIELD_MAPPINGS(evt)
        FIELD_MAP(event_t, num, LINX_FIELD_TYPE_UINT64)
        FIELD_MAP(event_t, time, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, type, LINX_FIELD_TYPE_CHARBUF_ARRAY)
        FIELD_MAP(event_t, args, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, rawarg, LINX_FIELD_TYPE_STRUCT)
        FIELD_MAP(event_t, arg, LINX_FIELD_TYPE_STRUCT)
        FIELD_MAP(event_t, res, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, rawres, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, failed, LINX_FIELD_TYPE_BOOL)
        FIELD_MAP(event_t, dir, LINX_FIELD_TYPE_CHARBUF)
    END_FIELD_MAPPINGS(evt)

    ret = linx_hash_map_add_field_batch("evt", evt_mappings, evt_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
        return -1;
    }

    return ret;
}

static void rich_event_clean(linx_event_t *event)
{
    for (uint32_t i = 0; i < g_linx_event_table[event->type].nparams; ++i) {
        switch (g_linx_event_table[event->type].params[i].type) {
        case LINX_FIELD_TYPE_UID:
        case LINX_FIELD_TYPE_PID:
            free(evt.arg.data[i]);
            evt.arg.data[i] = evt.rawarg.data[i] = NULL;
            break;
        default:
            break;
        }
    }
}

static void rich_event_args(linx_event_t *event)
{
    uint64_t size = 0;
    void *base = (void *)event + LINX_EVENT_HEADER_SIZE;

    evt.args = (char *)base;

    for (uint32_t i = 0; i < g_linx_event_table[event->type].nparams; ++i) {
        switch (g_linx_event_table[event->type].params[i].type) {
        case LINX_FIELD_TYPE_UID:
            struct passwd *pw = getpwuid((uid_t)(*(uint32_t *)(base + size)));
            if (pw) {
                evt.arg.data[i] = evt.rawarg.data[i] = 
                    strdup(pw->pw_name);
            } else {
                evt.arg.data[i] = evt.rawarg.data[i] = 
                    strdup("unknown");
            }
            break;
        case LINX_FIELD_TYPE_PID:
            linx_process_info_t *info = linx_process_cache_get((pid_t)(*(int64_t *)(base + size)));
            if (info) {
                evt.arg.data[i] = evt.rawarg.data[i] = 
                    strdup(info->comm);
            } else {
                evt.arg.data[i] = evt.rawarg.data[i] = 
                    strdup("unknown");
            }
            break;
        default:
            evt.arg.data[i] = evt.rawarg.data[i] = base + size;
            break;
        }

        size += event->params_size[i];
    }
}

static void rich_execve_exit(linx_event_t *event)
{
    linx_process_info_t *info = malloc(sizeof(linx_process_info_t));
    if (!info) {
        return;
    }

    memset(info, 0, sizeof(linx_process_info_t));

    info->pid = (pid_t)event->pid;
    info->ppid = (pid_t)event->ppid;
    info->create_time = time(NULL);
    info->update_time = info->create_time;
    info->is_alive = true;
    info->is_rich = true;
    info->state = LINX_PROCESS_STATE_RUNNING;

    memcpy(info->name, event->comm, strlen(event->comm));
    memcpy(info->comm, event->comm, strlen(event->comm));
    memcpy(info->cmdline, event->cmdline, strlen(event->cmdline));

    linx_process_cache_update(info);
}

int linx_event_rich_init(void)
{
    int ret = linx_event_rich_bind_field();

    return ret;
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
    int ret;

    rich_event_clean(event);

    snprintf(evt.time + len, sizeof(evt.time) - len, ".%09lu", remaining_ns);

    evt.num = event->type;
    event->type % 2 ? 
        strcpy(evt.dir, "<") : 
        strcpy(evt.dir, ">");
    evt.type = (char *)g_linx_event_table[event->type].name;
    evt.rawres = (int64_t)event->res;
    if (evt.rawres == 0) {
        evt.failed = false;
        strcpy(evt.res, "SUCCESS");
    } else {
        evt.failed = true;
        strcpy(evt.res, "ERRNO");
    }

    /**
     * 更新事件参数相关内容
    */
    rich_event_args(event);

    /**
     * 根据不同的事件，进行不同的上下文丰富
    */
    switch (event->type) {
        case LINX_EVENT_TYPE_EXECVE_X:
            rich_execve_exit(event);
            break;
        default:
            break;
    }

    ret = update_field_base(event->pid);

    return ret;
}

int linx_event_rich_deinit(void)
{
    return 0;
}

event_t *linx_event_rich_get(void)
{
    return &evt;
}
