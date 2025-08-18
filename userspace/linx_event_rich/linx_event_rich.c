#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

#include "linx_event_rich.h"
#include "linx_event_get.h"
#include "linx_hash_map.h"
#include "linx_log.h"

#include "linx_event_table.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_fd_info.h"

static event_t evt = {0};

static int update_field_base(linx_event_t *event, int64_t fd)
{
    field_update_table_t tables[] = {
        {"evt", &evt},
        {"fd", (void *)linx_process_cache_get_fd((pid_t)event->pid, fd)},
        {"proc", (void *)linx_process_cache_get((pid_t)event->pid)},
        {"user", (void *)linx_machine_status_get_user()},
        {"group", (void *)linx_machine_status_get_group()},
    };

    return linx_hash_map_update_tables_base(tables, sizeof(tables) / sizeof(tables[0]));
}

static int bind_field_evt(void)
{
    int ret;

    BEGIN_FIELD_MAPPINGS(evt)
        FIELD_MAP(event_t, num, LINX_FIELD_TYPE_UINT64)
        FIELD_MAP(event_t, time, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, type, LINX_FIELD_TYPE_CHARBUF_ARRAY)
        FIELD_MAP(event_t, args, LINX_FIELD_TYPE_CHARBUF_ARRAY)
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

static int linx_event_rich_bind_field(void)
{
    int ret = bind_field_evt();
    return ret;
}

static void rich_event_clean(linx_event_type_t type)
{
    if (type < 0 || type >= LINX_EVENT_TYPE_MAX) {
        return;
    }

    for (uint32_t i = 0; i < g_linx_event_table[type].nparams; ++i) {
        switch (g_linx_event_table[type].params[i].type) {
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
    uint64_t args_size = event->size - LINX_EVENT_HEADER_SIZE;
    void *base = (void *)event + LINX_EVENT_HEADER_SIZE;

    evt.args = realloc(evt.args, args_size);
    memcpy(evt.args, base, args_size);

    for (uint64_t i = 0; i < args_size; ++i) {
        if (evt.args[i] == '\0') {
            evt.args[i] = ' ';
        }
    }

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
                    strdup(info->name);
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

static char *parse_dirfd(linx_event_t *event, char *name, int64_t dirfd)
{
    linx_process_info_t *info;

    if (name != NULL && name[0] == '/') {
        return "";
    }

    if (dirfd == AT_FDCWD) {
        info = linx_process_cache_get((pid_t)event->pid);
        if (info) {
            return info->cwd;
        }
    }

    return "";
}

static void rich_store_event(linx_event_t *event)
{
    pid_t pid = (pid_t)event->pid;
    int64_t fd = -1;

    switch (event->type) {
    case LINX_EVENT_TYPE_READ_E:
    case LINX_EVENT_TYPE_OPENAT_E:
        fd = *(int64_t *)linx_event_get_param(event, 0);
        break;
    default:
        break;
    }

    if (fd == -1) {
        linx_process_cache_get(pid);
    } else {
        linx_process_cache_get_fd(pid, fd);
    }
}

/**
 * 返回fd
 */
static int64_t rich_open_openat_exit(linx_event_t *event)
{
    int64_t dirfd;
    char *name, *sdir;
    bool need_update = false;
    linx_fd_info_t *fd_info;

    fd_info = linx_process_cache_get_fd((pid_t)event->pid, (int64_t)event->res);
    if (!fd_info) {
        fd_info = calloc(1, sizeof(linx_fd_info_t));
        if (!fd_info) {
            return -1;
        }

        need_update = true;
    }

    if (event->type == LINX_EVENT_TYPE_OPENAT_X) {
        name = linx_event_get_param(event, 2);
        dirfd = *(int64_t *)linx_event_get_param(event, 1);

        sdir = parse_dirfd(event, name, dirfd);
    }

    snprintf(fd_info->filename, sizeof(fd_info->filename), "%s", name);
    snprintf(fd_info->directory, sizeof(fd_info->directory), "%s", sdir);
    if (strlen(sdir)) {
        snprintf(fd_info->name, sizeof(fd_info->name), "%s/%s", fd_info->directory, fd_info->filename);
    } else {
        snprintf(fd_info->name, sizeof(fd_info->name), "%s", fd_info->filename);
    }

    if (need_update) {
        fd_info->num = (int64_t)event->res;
        linx_process_cache_update_fd((pid_t)event->pid, fd_info);
    }

    return (int64_t)event->res;
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
    memcpy(info->cmdline, event->cmdline, strlen(event->cmdline));
    memcpy(info->args, event->cmdline + strlen(event->comm), sizeof(info->args));

    linx_process_cache_update(info);
}

static void rich_rw_exit(linx_event_t *event)
{
    int64_t fd;
    linx_fd_info_t *fd_info;

    if (event->type != LINX_EVENT_TYPE_SENDTO_X) {
        fd = *(int64_t *)linx_event_get_param(event, 2);
    }

    fd_info = linx_process_cache_get_fd((pid_t)event->pid, fd);
    if (!fd_info) {
        return;
    }
}

int linx_event_rich_init(void)
{
    int ret = linx_event_rich_bind_field();

    evt.last_event_type = -1;

    return ret;
}

void linx_event_rich_deinit(void)
{
    rich_event_clean(evt.last_event_type);
}

int linx_event_rich(linx_event_t *event)
{
    /**
     * 根据event事件类型
     * 判断是否有改变工作目录，改变用户等操作
     * 同步更新到应用层保存的结构体中
    */

    /* 更新 evt 结构体相关内容 */
    int64_t fd = -1;
    uint64_t ns = event->time;
    uint64_t remaining_ns = ns % 1000000000;
    time_t seconds = ns / 1000000000;
    struct tm *timeinfo = localtime(&seconds);
    size_t len = strftime(evt.time, sizeof(evt.time), "%Y-%m-%d %H:%M:%S", timeinfo);
    int ret;

    rich_event_clean(evt.last_event_type);
    evt.last_event_type = event->type;

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
        case LINX_EVENT_TYPE_SENDTO_E:

        case LINX_EVENT_TYPE_READ_E:
        case LINX_EVENT_TYPE_OPEN_E:
        case LINX_EVENT_TYPE_OPENAT_E:
            rich_store_event(event);
            break;
        case LINX_EVENT_TYPE_OPEN_X:
        case LINX_EVENT_TYPE_OPENAT_X:
            fd = rich_open_openat_exit(event);
            break;
        case LINX_EVENT_TYPE_EXECVE_X:
            rich_execve_exit(event);
            break;
        case LINX_EVENT_TYPE_READ_X:
        case LINX_EVENT_TYPE_WRITE_X:
        case LINX_EVENT_TYPE_SENDTO_X:
            rich_rw_exit(event);
            break;
        default:
            break;
    }

    ret = update_field_base(event, fd);

    return ret;
}

event_t *linx_event_rich_get(void)
{
    return &evt;
}
