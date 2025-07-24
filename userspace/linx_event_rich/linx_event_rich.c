#include <time.h>

#include "linx_event_rich.h"
#include "linx_hash_map.h"
#include "linx_log.h"
#include "event.h"

#include "linx_syscall_table.h"
#include "linx_process_cache.h"

static event_t evt = {0};

static int update_field_base(pid_t pid)
{
    field_update_table_t tables[] = {
        {"evt", &evt},
        {"proc", (void *)linx_process_cache_get(pid)}
    };

    return linx_hash_map_update_tables_base(tables, sizeof(tables) / sizeof(tables[0]));
}

static int linx_event_rich_bind_field(void)
{
    int ret;

    BEGIN_FIELD_MAPPINGS(evt)
        FIELD_MAP(event_t, num, FIELD_TYPE_UINT64)
        FIELD_MAP(event_t, time, FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, type, FILED_TYPE_CHARBUF_ARRAY)
        FIELD_MAP(event_t, args, FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, res, FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, rawres, FIELD_TYPE_CHARBUF)
        FIELD_MAP(event_t, failed, FIELD_TYPE_BOOL)
        FIELD_MAP(event_t, dir, FIELD_TYPE_CHARBUF)
    END_FIELD_MAPPINGS(evt)

    ret = linx_hash_map_add_field_batch("evt", evt_mappings, evt_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
        return -1;
    }
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

    /* 处理进程相关的系统调用事件 */
    switch (event->syscall_id) {
        case 57:  /* __NR_fork */
        case 58:  /* __NR_vfork */
            if (event->type == LINX_SYSCALL_TYPE_EXIT && event->res > 0) {
                /* fork成功，event->res是新进程的PID */
                linx_process_cache_on_fork((pid_t)event->res, event->pid);
            }
            break;
            
        case 59:  /* __NR_execve */
        case 322: /* __NR_execveat */
            if (event->type == LINX_SYSCALL_TYPE_EXIT && event->res == 0) {
                /* exec成功 */
                linx_process_cache_on_exec(event->pid, NULL, NULL, NULL);
            }
            break;
            
        case 60:  /* __NR_exit */
        case 231: /* __NR_exit_group */
            if (event->type == LINX_SYSCALL_TYPE_ENTER) {
                /* 进程即将退出 */
                int exit_code = 0;
                /* 这里可以从事件参数中获取退出码，如果eBPF提供了的话 */
                linx_process_cache_on_exit(event->pid, exit_code);
            }
            break;
            
        default:
            /* 对于其他系统调用，预加载进程信息以备后用 */
            linx_process_cache_preload(event->pid);
            break;
    }

    /* 更新 evt 结构体相关内容 */
    uint64_t ns = event->time;
    uint64_t remaining_ns = ns % 1000000000;
    time_t seconds = ns / 1000000000;
    struct tm *timeinfo = localtime(&seconds);
    size_t len = strftime(evt.time, sizeof(evt.time), "%Y-%m-%d %H:%M:%S", timeinfo);
    int ret;

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

    ret = update_field_base(event->pid);

    return ret;
}

int linx_event_rich_deinit(void)
{
    return 0;
}