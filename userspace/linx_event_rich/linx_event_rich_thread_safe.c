#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "linx_event_rich.h"
#include "linx_event_get.h"
#include "linx_hash_map_thread_safe.h"
#include "linx_log.h"

#include "linx_event_table.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_fd_info.h"

/* 线程本地的事件结构 */
static __thread event_t thread_local_evt = {0};

/**
 * 线程安全的字段基地址更新函数
 */
static int update_field_base_thread_safe(linx_event_t *event, int64_t fd)
{
    field_update_table_t tables[] = {
        {"evt", &thread_local_evt},
        {"fd", (void *)linx_process_cache_get_fd((pid_t)event->pid, fd)},
        {"proc", (void *)linx_process_cache_get((pid_t)event->pid)},
        {"user", (void *)linx_machine_status_get_user()},
        {"group", (void *)linx_machine_status_get_group()},
    };

    return linx_hash_map_thread_local_update_tables_base(tables, sizeof(tables) / sizeof(tables[0]));
}

/**
 * 线程安全的事件丰富处理
 */
int linx_event_rich_thread_safe(linx_event_t *event)
{
    /* 确保当前线程已注册 */
    if (linx_hash_map_register_thread() != 0) {
        LINX_LOG_ERROR("Failed to register thread for hash map access");
        return -1;
    }
    
    /**
     * 根据event事件类型
     * 判断是否有改变工作目录，改变用户等操作
     * 同步更新到应用层保存的结构体中
    */

    /* 更新 thread_local_evt 结构体相关内容 */
    int64_t fd = -1;
    uint64_t ns = event->time;
    uint64_t remaining_ns = ns % 1000000000;
    time_t seconds = ns / 1000000000;
    struct tm *timeinfo = localtime(&seconds);
    size_t len = strftime(thread_local_evt.time, sizeof(thread_local_evt.time), "%Y-%m-%d %H:%M:%S", timeinfo);
    int ret;

    // 清理之前的事件数据
    if (thread_local_evt.last_event_type >= 0 && thread_local_evt.last_event_type < LINX_EVENT_TYPE_MAX) {
        for (uint32_t i = 0; i < g_linx_event_table[thread_local_evt.last_event_type].nparams; ++i) {
            switch (g_linx_event_table[thread_local_evt.last_event_type].params[i].type) {
            case LINX_FIELD_TYPE_UID:
            case LINX_FIELD_TYPE_PID:
                free(thread_local_evt.arg[i].data);
                /* fall through */
            default:
                thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = NULL;
                thread_local_evt.arg[i].size = thread_local_evt.rawarg[i].size = 0;
                break;
            }
        }
    }
    
    thread_local_evt.last_event_type = event->type;

    snprintf(thread_local_evt.time + len, sizeof(thread_local_evt.time) - len, ".%09lu", remaining_ns);

    thread_local_evt.num = event->type;
    event->type % 2 ? 
        strcpy(thread_local_evt.dir, "<") : 
        strcpy(thread_local_evt.dir, ">");
    thread_local_evt.type = (char *)g_linx_event_table[event->type].name;
    thread_local_evt.rawres = (int64_t)event->res;
    if (thread_local_evt.rawres == 0) {
        thread_local_evt.failed = false;
        strcpy(thread_local_evt.res, "SUCCESS");
    } else {
        thread_local_evt.failed = true;
        strcpy(thread_local_evt.res, "ERRNO");
    }

    /**
     * 更新事件参数相关内容 (简化版本，完整实现需要移植所有rich_event_args逻辑)
    */
    uint64_t size = 0;
    uint64_t args_size = event->size - LINX_EVENT_HEADER_SIZE;
    void *base = (void *)event + LINX_EVENT_HEADER_SIZE;

    thread_local_evt.args = realloc(thread_local_evt.args, args_size);
    memcpy(thread_local_evt.args, base, args_size);

    for (uint64_t i = 0; i < args_size; ++i) {
        if (thread_local_evt.args[i] == '\0') {
            thread_local_evt.args[i] = ' ';
        }
    }

    for (uint32_t i = 0; i < g_linx_event_table[event->type].nparams; ++i) {
        switch (g_linx_event_table[event->type].params[i].type) {
        case LINX_FIELD_TYPE_UID:
            {
                struct passwd *pw = getpwuid((uid_t)(*(uint32_t *)(base + size)));
                if (pw) {
                    thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = 
                        strdup(pw->pw_name);
                } else {
                    thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = 
                        strdup("unknown");
                }

                thread_local_evt.arg[i].size = thread_local_evt.rawarg[i].size = 
                    strlen(thread_local_evt.arg[i].data);
            }
            break;
        case LINX_FIELD_TYPE_PID:
            {
                linx_process_info_t *info = linx_process_cache_get((pid_t)(*(int64_t *)(base + size)));
                if (info) {
                    thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = 
                        strdup(info->name);
                } else {
                    thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = 
                        strdup("unknown");
                }

                thread_local_evt.arg[i].size = thread_local_evt.rawarg[i].size = 
                    strlen(thread_local_evt.arg[i].data);
            }
            break;
        default:
            thread_local_evt.arg[i].data = thread_local_evt.rawarg[i].data = base + size;
            thread_local_evt.arg[i].size = thread_local_evt.rawarg[i].size = event->params_size[i];
            break;
        }

        size += event->params_size[i];
    }

    /**
     * 根据不同的事件，进行不同的上下文丰富
     * (这里需要根据具体需求移植相关逻辑)
    */
    switch (event->type) {
        case LINX_EVENT_TYPE_SENDTO_E:
        case LINX_EVENT_TYPE_READ_E:
        case LINX_EVENT_TYPE_OPEN_E:
        case LINX_EVENT_TYPE_OPENAT_E:
            // 存储事件信息
            memcpy(thread_local_evt.last_event, event, event->size);
            break;
        case LINX_EVENT_TYPE_OPEN_X:
        case LINX_EVENT_TYPE_OPENAT_X:
            fd = (int64_t)event->res;
            break;
        case LINX_EVENT_TYPE_EXECVE_X:
            // 处理execve退出事件
            break;
        case LINX_EVENT_TYPE_READ_X:
        case LINX_EVENT_TYPE_WRITE_X:
        case LINX_EVENT_TYPE_SENDTO_X:
        case LINX_EVENT_TYPE_RECVFROM_X:
            // 处理读写事件
            break;
        case LINX_EVENT_TYPE_DUP_X:
        case LINX_EVENT_TYPE_DUP2_X:
        case LINX_EVENT_TYPE_DUP3_X:
            fd = (int64_t)event->res;
            break;
        default:
            break;
    }

    ret = update_field_base_thread_safe(event, fd);

    return ret;
}

/**
 * 获取线程本地的事件结构
 */
event_t *linx_event_rich_get_thread_safe(void)
{
    return &thread_local_evt;
}

/**
 * 线程清理函数
 */
void linx_event_rich_thread_cleanup(void)
{
    if (thread_local_evt.last_event_type >= 0 && thread_local_evt.last_event_type < LINX_EVENT_TYPE_MAX) {
        for (uint32_t i = 0; i < g_linx_event_table[thread_local_evt.last_event_type].nparams; ++i) {
            switch (g_linx_event_table[thread_local_evt.last_event_type].params[i].type) {
            case LINX_FIELD_TYPE_UID:
            case LINX_FIELD_TYPE_PID:
                free(thread_local_evt.arg[i].data);
                break;
            default:
                break;
            }
        }
    }
    
    free(thread_local_evt.args);
    thread_local_evt.args = NULL;
    
    linx_hash_map_unregister_thread();
}