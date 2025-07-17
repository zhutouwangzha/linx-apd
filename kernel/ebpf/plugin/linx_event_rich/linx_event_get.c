#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>

#include "linx_common.h"
#include "linx_event_get.h"
#include "linx_syscall_table.h"
#include "plugin_config.h"

int linx_event_get_fullpath(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;
    
    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", event->fullpath) < 0) {
        LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                         strlen(event->fullpath), event->fullpath, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_time(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;
    uint64_t ns = event->time;
    uint64_t remaining_ns = ns % 1000000000UL;
    time_t seconds = ns / 1000000000UL;
    struct tm *tm_info = localtime(&seconds);
    size_t len = strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);

    if (LINX_SNPRINTF(byte_write, buf + len, buf_size - len, ".%09lu", remaining_ns) < 0) {
        LINX_LOG_WARNING("Failed when copy '.%09lu' str to %lu bytes buf!", 
                         remaining_ns, buf_size - len);
        return -1;
    }

    return byte_write + len;
}

int linx_event_get_syscall(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, 
                      "%d(%s)", event->syscall_id,
                      g_linx_syscall_table[event->syscall_id].name) < 0)
    {
        LINX_LOG_WARNING("Failed when copy '%d(%s)' str to %lu bytes buf!", 
                         event->syscall_id,
                         g_linx_syscall_table[event->syscall_id].name,
                         buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_syscall_name(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, 
                      "%s", g_linx_syscall_table[event->syscall_id].name) < 0)
    {
        LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                         strlen(g_linx_syscall_table[event->syscall_id].name), 
                         g_linx_syscall_table[event->syscall_id].name, 
                         buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_syscall_id(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%d", event->syscall_id) < 0) {
        LINX_LOG_WARNING("Failed when copy '%d' str to %lu bytes buf!", 
                          event->syscall_id, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_res(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%ld", (int64_t)event->res) < 0) {
        LINX_LOG_WARNING("Failed when copy '%ld' str to %lu bytes buf!", 
                         (int64_t)event->res, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_comm(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", event->comm) < 0) {
        LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                         strlen(event->comm), 
                         event->comm, 
                         buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_cmdline(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", event->cmdline) < 0) {
        LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                         strlen(event->cmdline), 
                         event->cmdline, 
                         buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_pid(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%lu", event->pid) < 0) {
        LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                         event->pid, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_tid(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%lu", event->tid) < 0) {
        LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                         event->tid, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_ppid_id(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%lu", event->ppid) < 0) {
        LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                         event->ppid, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_ppid_name(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", event->p_fullpath) < 0) {
        LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                         strlen(event->p_fullpath), 
                         event->p_fullpath, 
                         buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_ppid(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0, ret;

    ret = linx_event_get_ppid_id(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get ppid_id!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, "(") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes '(' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    ret = linx_event_get_ppid_name(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get ppid_name!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, ")") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes ')' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    return byte_write;
}

int linx_event_get_user_id(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%lu", event->uid) < 0) {
        LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                         event->uid, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_user(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    struct passwd *pw = getpwuid(event->uid);
    if (!pw) {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, "unknow") < 0) {
            LINX_LOG_WARNING("Failed when copy 6 bytes 'unknow' str to %lu bytes buf!", buf_size);
            return -1;
        }
    } else {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", pw->pw_name) < 0) {
            LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                             strlen(pw->pw_name), pw->pw_name, buf_size);
            return -1;
        }
    }

    return byte_write;
}

int linx_event_get_user_all(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0, ret;

    ret = linx_event_get_user_id(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get user_id!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, "(") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes '(' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    ret = linx_event_get_user(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get user_name!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, ")") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes ')' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    return byte_write;
}

int linx_event_get_group_id(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (LINX_SNPRINTF(byte_write, buf, buf_size, "%lu", event->gid) < 0) {
        LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                         event->gid, buf_size);
        return -1;
    }

    return byte_write;
}

int linx_event_get_group(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    struct group *gr = getgrgid(event->gid);
    if (!gr) {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, "unknow") < 0) {
            LINX_LOG_WARNING("Failed when copy 6 bytes 'unknow' str to %lu bytes buf!", buf_size);
            return -1;
        }
    } else {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, "%s", gr->gr_name) < 0) {
            LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                            strlen(gr->gr_name), gr->gr_name, buf_size);
            return -1;
        }
    }

    return byte_write;
}

int linx_event_get_group_all(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0, ret;

    ret = linx_event_get_group_id(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get group_id!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, "(") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes '(' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    ret = linx_event_get_group(event, buf + byte_write, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get group_name!");
        return ret;
    } else {
        byte_write += ret;
    }

    if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, ")") < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes ')' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    return byte_write;
}

int linx_event_get_fds_id(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    for (int i = 0; i < event->nfds; ++i) {
        if (LINX_SNPRINTF(byte_write, 
                          buf + byte_write, 
                          buf_size - byte_write, 
                          "%d", event->fds[i]) < 0) 
        {
            LINX_LOG_WARNING("Failed when copy '%d' str to %lu bytes buf for the %d time!", 
                             event->fds[i], buf_size - byte_write, i);
            return -1;
        }

        if (i != event->nfds - 1) {
            if (LINX_SNPRINTF(byte_write, 
                              buf + byte_write, 
                              buf_size - byte_write, 
                              ", ") < 0) 
            {
                LINX_LOG_WARNING("Failed when copy 2 bytes ', ' str to %lu bytes buf for %d time!", 
                                 buf_size - byte_write, i);
                return -1;
            }
        }
    }

    return byte_write;
}

int linx_event_get_fds_name(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    for (int i = 0; i < event->nfds; ++i) {
        if (LINX_SNPRINTF(byte_write, 
                          buf + byte_write, 
                          buf_size - byte_write, 
                          "%s", event->fd_path[i]) < 0) 
        {
            LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf for %d time!", 
                             strlen(event->fd_path[i]), event->fd_path[i], 
                             buf_size - byte_write, i);
            return -1;
        }

        if (i != event->nfds - 1) {
            if (LINX_SNPRINTF(byte_write, 
                              buf + byte_write, 
                              buf_size - byte_write, 
                              ", ") < 0) 
            {
                LINX_LOG_WARNING("Failed when copy 2 bytes ', ' str to %lu bytes buf for the %d time!", 
                                 buf_size - byte_write, i);
                return -1;
            }
        }
    }

    return byte_write;
}

int linx_event_get_fds(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    for (int i = 0; i < event->nfds; ++i) {
        if (LINX_SNPRINTF(byte_write, 
                          buf + byte_write, 
                          buf_size - byte_write, 
                          "%d(%s)", 
                          event->fds[i],
                          event->fd_path[i]) < 0) 
        {
            LINX_LOG_WARNING("Failed when copy '%d(%s)' str to %lu bytes buf for the %d time!", 
                             event->fds[i], event->fd_path[i], 
                             buf_size - byte_write, i);
            return -1;
        }

        if (i != event->nfds - 1) {
            if (LINX_SNPRINTF(byte_write, 
                              buf + byte_write, 
                              buf_size - byte_write, 
                              ", ") < 0) 
            {
                LINX_LOG_WARNING("Failed when copy 2 bytes ', ' str to %lu bytes buf for the %d time!", 
                                 buf_size - byte_write, i);
                return -1;
            }
        }
    }

    return byte_write;
}

int get_syscall_enter_params(uint64_t syscall_id, int idx, int *byte_write,
                             uint8_t **data, char *buf, size_t buf_size)
{
    uint64_t str_len;

    switch (g_linx_syscall_table[syscall_id].enter_type[idx]) {
    case LINX_TYPE_NONE: /* 哪些结构体指针默认以uint64_t将地址显示出来 */
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%lu", *(uint64_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                             *(uint64_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(uint64_t);
        break;
    case LINX_TYPE_INT8:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%hhd", *(int8_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%hhd' str to %lu bytes buf!", 
                             *(int8_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(int8_t);
        break;
    case LINX_TYPE_INT16:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%hd", *(int16_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%hd' str to %lu bytes buf!", 
                             *(int16_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(int16_t);
        break;
    case LINX_TYPE_INT32:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%d", *(int32_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%d' str to %lu bytes buf!", 
                             *(int32_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(int32_t);
        break;
    case LINX_TYPE_INT64:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%ld", *(int64_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%ld' str to %lu bytes buf!", 
                             *(int64_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(int64_t);
        break;
    case LINX_TYPE_UINT8:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%hhu", *(uint8_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%hhu' str to %lu bytes buf!", 
                             *(uint8_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(uint8_t);
        break;
    case LINX_TYPE_UINT16:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%hu", *(uint16_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%hu' str to %lu bytes buf!", 
                             *(uint16_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(uint16_t);
        break;
    case LINX_TYPE_UINT32:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%u", *(uint32_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%u' str to %lu bytes buf!", 
                             *(uint32_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(uint32_t);
        break;
    case LINX_TYPE_UINT64:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%lu", *(uint64_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%lu' str to %lu bytes buf!", 
                             *(uint64_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(uint64_t);
        break;
    case LINX_TYPE_CHARBUF:
    case LINX_TYPE_BYTEBUF:
        str_len = strlen((char *)(*data));
        if (str_len > g_plugin_config.init_config.str_max_size) {
            if (LINX_SNPRINTF(*byte_write, 
                              buf + *byte_write, 
                              buf_size, 
                              "%.*s...", 
                              (int)g_plugin_config.init_config.str_max_size, 
                              (char *)(*data)) < 0) 
            {
                LINX_LOG_WARNING("Failed when copy %d bytes '%.s...' str to %lu bytes buf!", 
                                 (int)g_plugin_config.init_config.str_max_size + 3, 
                                 (int)g_plugin_config.init_config.str_max_size, 
                                 (char *)(*data),
                                 buf_size);
                return -1;
            }
        } else {
            if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%s", (char *)(*data)) < 0) {
                LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf!", 
                                 str_len, (char *)(*data), buf_size);
                return -1;
            }
        }
        (*data) += (str_len + 1);
        break;
    default:
        LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                         g_linx_syscall_table[syscall_id].enter_type[idx]);
        return -1;
        break;
    }

    return 0;
}

int get_syscall_exit_params(uint64_t syscall_id, int idx, int *byte_write,
                            uint8_t **data, char *buf, size_t buf_size)
{
    switch (g_linx_syscall_table[syscall_id].exit_type[idx]) {
    case LINX_TYPE_INT64:
        if (LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size, "%ld", *(int64_t *)(*data)) < 0) {
            LINX_LOG_WARNING("Failed when copy '%ld' str to %lu bytes buf!", 
                             *(int64_t *)(*data), buf_size);
            return -1;
        }
        (*data) += sizeof(int64_t);
        break;
    default:
        LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                         g_linx_syscall_table[syscall_id].enter_type[idx]);
        return -1;
        break;
    }

    return 0;
}

int linx_event_get_args(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;
    uint8_t *data = ((uint8_t *)event) + LINX_EVENT_HEADER_SIZE;

    if (LINX_SNPRINTF(byte_write, 
                      buf + byte_write, 
                      buf_size - byte_write, 
                      "(") < 0) 
    {
        LINX_LOG_WARNING("Failed when copy 1 bytes '(' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return -1;
    }

    for (int i = 0; 
        i < g_linx_syscall_table[event->syscall_id].enter_num;
        ++i)
    {
        if (LINX_SNPRINTF(byte_write, 
                          buf + byte_write, 
                          buf_size - byte_write,
                          "%s=", 
                          g_linx_syscall_table[event->syscall_id].param_name[i]) < 0)
        {
            LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf for %d time!",
                             strlen(g_linx_syscall_table[event->syscall_id].param_name[i]), 
                             g_linx_syscall_table[event->syscall_id].param_name[i], 
                             buf_size - byte_write, i);
            return -1;
        }

        if (get_syscall_enter_params(event->syscall_id, i, 
                                     &byte_write, &data, 
                                     buf, buf_size - byte_write)) 
        {
            LINX_LOG_WARNING("Failed to get the %d param(%s) of the %s syscall!",
                             i, g_linx_syscall_table[event->syscall_id].param_name[i],
                             g_linx_syscall_table[event->syscall_id].name);
            return -1;
        }

        if (i == g_linx_syscall_table[event->syscall_id].enter_num - 1) {
            if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, ")") < 0) {
                LINX_LOG_WARNING("Failed when copy 1 bytes ')' str to %lu bytes buf for %d time!", 
                                 buf_size - byte_write, i);
                return -1;
            }
        } else {
            if (LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, ", ") < 0) {
                LINX_LOG_WARNING("Failed when copy 2 bytes ', ' str to %lu bytes buf for %d time!", 
                                 buf_size - byte_write, i);
                return -1;
            }
        }
    }

    /**
     * 不再这里获取系统调用返回值
     * 通过linx_event_get_res获取
     */
    // if (get_syscall_exit_params(event->syscall_id, 0, &byte_write, &data, buf, buf_size)) {
    //     return -1;
    // }

    buf[byte_write] = '\0';
    return byte_write;
}

int linx_event_get_dir(linx_event_t *event, char *buf, size_t buf_size)
{
    int byte_write = 0;

    if (event->type == LINX_SYSCALL_TYPE_ENTER) {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, ">") < 0) {
            LINX_LOG_WARNING("Failed when copy 1 bytes '>' str to %lu bytes buf!", 
                             buf_size);
            return -1;
        }
    } else {
        if (LINX_SNPRINTF(byte_write, buf, buf_size, "<") < 0) {
            LINX_LOG_WARNING("Failed when copy 1 bytes '<' str to %lu bytes buf!", 
                             buf_size);
            return -1;
        }
    }

    return byte_write;
}
