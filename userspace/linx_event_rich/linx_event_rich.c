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

    return ret;
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

    snprintf(evt.time + len, sizeof(evt.time) - len, ".%09lu", remaining_ns);

    if (event->syscall_id == LINX_SYSCALL_EXECVE)
    {
        if (event->type == LINX_SYSCALL_TYPE_ENTER) {
            // ENTER事件：预先同步更新进程缓存，为后续EXIT事件做准备
            printf("DEBUG: EXECVE ENTER - PID:%lu COMM:%s CMDLINE:%s\n", 
                   event->pid, event->comm, event->cmdline);
            ret = linx_process_cache_update_sync(event->pid);
        } else {
            // EXIT事件：进程可能已退出，尝试从缓存获取信息
            printf("DEBUG: EXECVE EXIT - PID:%lu COMM:%s CMDLINE:%s\n", 
                   event->pid, event->comm, event->cmdline);
            // 如果缓存中没有，说明是极短生命周期进程，尝试从事件中获取基本信息
            linx_process_info_t *cached_info = linx_process_cache_get(event->pid);
            if (!cached_info) {
                printf("DEBUG: No cached info found for PID %lu, creating from event data\n", event->pid);
                // 对于极短生命周期进程，尝试从事件数据中创建基本缓存项
                ret = linx_process_cache_create_from_event(event->pid, event->comm, event->cmdline);
            } else {
                printf("DEBUG: Found cached info for PID %lu: %s\n", event->pid, cached_info->name);
            }
        }
    }

    evt.num = event->syscall_id;
    event->type == LINX_SYSCALL_TYPE_ENTER ? 
        strcpy(evt.dir, ">") : 
        strcpy(evt.dir, "<");
    evt.type = (char *)g_linx_syscall_table[event->syscall_id].name;
    evt.rawres = (int64_t)event->res;
    if (evt.rawres == 0) {
        evt.failed = false;
        strcpy(evt.res, "SUCCESS");
    } else {
        evt.failed = true;
        strcpy(evt.res, "ERRNO");
    }
    
    // 对于EXECVE EXIT事件，确保进程信息在规则匹配前存在于缓存中
    if (event->syscall_id == LINX_SYSCALL_EXECVE && event->type == LINX_SYSCALL_TYPE_EXIT) {
        linx_process_info_t *proc_info = linx_process_cache_get(event->pid);
        if (!proc_info) {
            printf("DEBUG: EXECVE EXIT - No process cache found for PID %lu, creating from event\n", event->pid);
            linx_process_cache_create_from_event(event->pid, event->comm, event->cmdline);
        }
    }

    // 针对find命令进行特殊调试
    if (strcmp(event->comm, "find") == 0)
    {
        printf("DEBUG: Processing find command - PID:%lu DIR:%s\n", event->pid, evt.dir);
        
        // 确保进程信息在hash map更新前存在
        linx_process_info_t *proc_info = linx_process_cache_get(event->pid);
        if (!proc_info) {
            printf("DEBUG: find process not in cache, creating emergency cache entry\n");
            linx_process_cache_create_from_event(event->pid, event->comm, event->cmdline);
        } else {
            printf("DEBUG: find process found in cache: name=%s cmdline=%s\n", 
                   proc_info->name, proc_info->cmdline);
        }
    }

    ret = update_field_base(event->pid);

    return ret;
}

int linx_event_rich_deinit(void)
{
    return 0;
}