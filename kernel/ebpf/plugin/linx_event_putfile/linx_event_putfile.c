#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "linx_event_putfile.h"
#include "linx_event.h"
#include "linx_common.h"
#include "linx_event_get.h"

#include "plugin_log.h"
#include "plugin_config.h"

static linx_event_putfile_t g_linx_event_putfile = {0};

/**
 * 文件映射窗口
 */
#define LINX_EVENT_MAP_WINDOW_SIZE    (4 * 1024 * 1024)

/**
 * 一次性写入文件的最大长度
 */
#define LINX_EVENT_PUTLINE_MAX_LEN  (8 * 1024)

#define LINX_EVENT_PUTFILE_FUNC(name, event, byte_write, buf, buf_size)   \
    ({                                                                    \
        int putfile_ret;                                                  \
        do {                                                              \
            putfile_ret = LINX_SNPRINTF((byte_write),                     \
                                        (buf) + (byte_write),             \
                                        (buf_size), #name"=");            \
            if (putfile_ret < 0) {                                        \
                break;                                                    \
            }                                                             \
                                                                          \
            putfile_ret = linx_event_get_##name((event),                  \
                                                (buf) + (byte_write),     \
                                                (buf_size));              \
            if (putfile_ret < 0) {                                        \
                break;                                                    \
            } else {                                                      \
                (byte_write) += putfile_ret;                              \
            }                                                             \
                                                                          \
            putfile_ret = LINX_SNPRINTF((byte_write),                     \
                                        (buf) + (byte_write),             \
                                        (buf_size), ", ");                \
        } while (0);                                                      \
        putfile_ret;                                                      \
    })

static int linx_event_putfile_normal_time(linx_event_t *event, size_t *byte_write, 
                                          char *buf, size_t buf_size)
{
    int ret;

    ret = LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size - *byte_write, "[");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes '[' str to %lu bytes buf!", 
                         buf_size - *byte_write);
        return ret;
    }

    ret = linx_event_get_time(event, buf + *byte_write, buf_size - *byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get time!");
        return ret;
    } else {
        *byte_write += ret;
    }

    ret = LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size - *byte_write, "]: ");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 3 bytes ']: ' str to %lu bytes buf!", 
                         buf_size - *byte_write);
        return ret;
    }

    return ret;
}

static int linx_event_putfile_normal_sysname(linx_event_t *event, size_t *byte_write, 
                                             char *buf, size_t buf_size)
{
    int ret;

    ret = linx_event_get_syscall_name(event, buf + *byte_write, buf_size - *byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get syscall name!");
        return ret;
    } else {
        *byte_write += ret;
    }

    ret = LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size - *byte_write, ", ");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 2 bytes ', ' str to %lu bytes buf!", 
                         buf_size - *byte_write);
        return ret;
    }

    return ret;
}

static int linx_event_putfile_normal_fds(linx_event_t *event, size_t *byte_write, 
                                         char *buf, size_t buf_size)
{
    int ret;

    ret = LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size - *byte_write, "fds=[");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 5 bytes 'fds=[' str to %lu bytes buf!", 
                         buf_size - *byte_write);
        return ret;
    }

    ret = linx_event_get_fds(event, buf + *byte_write, buf_size - *byte_write);
    if (ret < 0) {
        LINX_LOG_WARNING("Failed to get fds!");
        return ret;
    } else {
        *byte_write += ret;
    }

    ret = LINX_SNPRINTF(*byte_write, buf + *byte_write, buf_size - *byte_write, "]");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes ']' str to %lu bytes buf!", 
                         buf_size - *byte_write);
        return ret;
    }

    return ret;
}

static int linx_event_putfile_write(int fd, char *buf, size_t buf_size)
{
    off_t window_end = g_linx_event_putfile.map_offset +
                       g_linx_event_putfile.map_size;
    off_t window_pos;

    /* 检查是否要移动映射窗口 */
    if (g_linx_event_putfile.write_offset + buf_size > 
        window_end)
    {
        munmap(g_linx_event_putfile.map, g_linx_event_putfile.map_size);

        g_linx_event_putfile.map_offset = ((g_linx_event_putfile.write_offset + buf_size) / 
                                           LINX_EVENT_MAP_WINDOW_SIZE) *
                                          LINX_EVENT_MAP_WINDOW_SIZE;

        ftruncate(fd, g_linx_event_putfile.map_offset + 
                  g_linx_event_putfile.map_size);

        g_linx_event_putfile.map = mmap(NULL, g_linx_event_putfile.map_size,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, g_linx_event_putfile.map_offset);
        if (g_linx_event_putfile.map == MAP_FAILED) {
            LINX_LOG_ERROR("Window move to %d failed!", g_linx_event_putfile.map_offset);
            return -1;
        }
    }

    window_pos = g_linx_event_putfile.write_offset - 
                 g_linx_event_putfile.map_offset;

    memcpy(g_linx_event_putfile.map + window_pos, buf, buf_size);
    g_linx_event_putfile.write_offset += buf_size;

    /* 定期异步刷盘 */
    if (window_pos % (512 * 1024) == 0) {
        msync(g_linx_event_putfile.map, window_pos + buf_size, MS_ASYNC);
    }

    return buf_size;
}

static void linx_event_putfile_normal(uint8_t *data)
{
    linx_event_t *event = (linx_event_t *)data;
    size_t byte_write = 0, buf_size = LINX_EVENT_PUTLINE_MAX_LEN;
    int ret;
    char *buf;

    LINX_MEM_CALLOC(char *, buf, 1, buf_size);
    if (!buf) {
        LINX_LOG_ERROR("Failed to allco for space for buf!");
        return;
    }

    ret = linx_event_putfile_normal_time(event, &byte_write, buf, buf_size);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile normal time!");
        return;
    }

    ret = linx_event_putfile_normal_sysname(event, &byte_write, buf, buf_size);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile normal sysname!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(syscall_id, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile syscall_id!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(dir, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile dir!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(res, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile res!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(user, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile user!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(comm, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile comm!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(cmdline, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile cmdline!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(pid, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile pid!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(tid, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile tid!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(ppid, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile ppid!");
        return;
    }

    ret = LINX_EVENT_PUTFILE_FUNC(args, event, byte_write, buf, buf_size - byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile args!");
        return;
    }

    ret = linx_event_putfile_normal_fds(event, &byte_write, buf, buf_size);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to putfile normal fds!");
        return;
    }

    ret = LINX_SNPRINTF(byte_write, buf + byte_write, buf_size - byte_write, "\n");
    if (ret < 0) {
        LINX_LOG_WARNING("Failed when copy 1 bytes '\\n' str to %lu bytes buf!", 
                         buf_size - byte_write);
        return;
    }

    ret = linx_event_putfile_write(g_plugin_config.init_config.output_config.fd, 
                                   buf, byte_write);
    if (ret < 0) {
        LINX_LOG_ERROR("Failed to write %d bytes '%s' str to %s file!",
                       byte_write, buf, 
                       g_plugin_config.init_config.output_config.path);
    }

    LINX_MEM_FREE(buf);
}

void linx_event_putfile(uint8_t *data)
{
    if (!g_plugin_config.init_config.output_config.put_flag) {
        LINX_LOG_DEBUG("The file write flag is not enabled!");
        return;
    }

    switch (g_plugin_config.init_config.output_config.format) {
    case 0: /* 普通模式 */
        LINX_LOG_DEBUG("Write the log to the file in normal mode!");
        linx_event_putfile_normal(data);
        break;
    case 1: /* json格式 */
        LINX_LOG_DEBUG("Write the log to the file in json mode!");
        break;
    default:
        LINX_LOG_WARNING("The current mode is not supported!");
        break;
    }
}

void linx_event_putfile_clean(void)
{
    msync(g_linx_event_putfile.map, 
          g_linx_event_putfile.write_offset - 
          g_linx_event_putfile.map_offset, 
          MS_SYNC);
    munmap(g_linx_event_putfile.map, 
           g_linx_event_putfile.map_size);
    ftruncate(g_plugin_config.init_config.output_config.fd,
              g_linx_event_putfile.write_offset);
    close(g_plugin_config.init_config.output_config.fd);
}

int linx_event_open_file(char *file, int *fd)
{
    struct stat st;
    off_t file_size;

    *fd = open(file, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (*fd == -1) {
        LINX_LOG_ERROR("The '%s' file does not exist!", file);
        return -1;
    }

    fstat(*fd, &st);
    file_size = st.st_size;
    g_linx_event_putfile.write_offset = file_size;
    g_linx_event_putfile.map_offset = (g_linx_event_putfile.write_offset / 
                                       LINX_EVENT_MAP_WINDOW_SIZE) *
                                      LINX_EVENT_MAP_WINDOW_SIZE;
    g_linx_event_putfile.map_size = LINX_EVENT_MAP_WINDOW_SIZE;

    if (ftruncate(*fd, g_linx_event_putfile.map_offset + 
                  g_linx_event_putfile.map_size) == -1)
    {
        LINX_LOG_ERROR("Failed to adjust the '%s' file size to %d!", file, 
                       g_linx_event_putfile.map_offset + g_linx_event_putfile.map_size);
        close(*fd);
        return -1;
    }

    g_linx_event_putfile.map = mmap(NULL, g_linx_event_putfile.map_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED, *fd, g_linx_event_putfile.map_offset);
    if (g_linx_event_putfile.map == MAP_FAILED) {
        LINX_LOG_ERROR("Mmap '%s' failed!", file);
        close(*fd);
        return -1;
    }

    return 0;
}
