#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <pthread.h>

#include "linx_process_cache.h"
#include "linx_hash_map.h"
#include "linx_log.h"

static linx_process_cache_t *g_process_cache = NULL;

static int linx_process_cache_bind_field(void)
{
    BEGIN_FIELD_MAPPINGS(proc)
        FIELD_MAP(linx_process_info_t, pid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, ppid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, pgid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, sid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, uid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, gid, FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, name, FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, comm, FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, cmdline, FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, exe, FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, cwd, FIELD_TYPE_CHARBUF)
    END_FIELD_MAPPINGS(proc)

    return linx_hash_map_add_field_batch("proc", proc_mappings, proc_mappings_count);
}

static int read_proc_stat(pid_t pid, linx_process_info_t *info)
{
    char path[PROC_PATH_MAX_LEN];
    char buffer[4096];
    FILE *fp;
    char state;
    char *start, *end;
    size_t name_len;
    int ret;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        fclose(fp);
        return -1;
    }

    start = strchr(buffer, '(');
    end = strrchr(buffer, ')');
    if (!start || !end || end <= start) {
        fclose(fp);
        return -1;
    }

    name_len = end - start - 1;
    if (name_len >= PROC_COMM_MAX_LEN) {
        name_len = PROC_COMM_MAX_LEN - 1;
    }

    strncpy(info->comm, start + 1, name_len);
    info->comm[name_len] = '\0';

    strncpy(info->name, info->comm, name_len + 1);

    ret = sscanf(end + 2, " %c %d %d %d %*d %*d %*u %*u %*u %*u %*u %lu %lu "
                 "%*d %*d %d %d %*d %*d %lu %lu %ld",
                 &state, &info->ppid, &info->pgid, &info->sid,
                 &info->utime, &info->stime,
                 &info->priority, &info->nice,
                 &info->start_time, &info->vsize,
                 &info->rss);
    if (ret < 10) {
        fclose(fp);
        return -1;
    }

    switch (state) {
    case 'R':
        info->state = LINX_PROCESS_STATE_RUNNING;
        break;
    case 'S':
        info->state = LINX_PROCESS_STATE_SLEEPING;
        break;
    case 'Z':
        info->state = LINX_PROCESS_STATE_ZOMBIE;
        break;
    case 'T':
        info->state = LINX_PROCESS_STATE_STOPPED;
        break;
    default:
        info->state = LINX_PROCESS_STATE_UNKNOWN;
        break;
    }

    info->rss *= getpagesize();

    fclose(fp);
    return 0;
}

static int read_proc_status(pid_t pid, linx_process_info_t *info)
{
    char path[PROC_PATH_MAX_LEN];
    char line[256];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &info->uid);
        } else if (strncmp(line, "Gid:", 4) == 0) {
            sscanf(line, "Gid:\t%d", &info->gid);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize:\t%lu kB", &info->vsize);
            info->vsize *= 1024; /* 转换为字节 */
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS:\t%lu kB", &info->rss);
            info->rss *= 1024; /* 转换为字节 */
        } else if (strncmp(line, "RssShmem:", 9) == 0) {
            sscanf(line, "RssShmem:\t%lu kB", &info->shared);
            info->shared *= 1024; /* 转换为字节 */
        }
    }
    
    fclose(fp);
    return 0;
}

static int read_proc_cmdline(pid_t pid, linx_process_info_t *info)
{
    char path[PROC_PATH_MAX_LEN];
    int fd;
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    len = read(fd, info->cmdline, PROC_CMDLINE_LEN - 1);
    if (len > 0) {
        info->cmdline[len] = '\0';
        /* 将NULL字符替换为空格 */
        for (ssize_t i = 0; i < len - 1; i++) {
            if (info->cmdline[i] == '\0') {
                info->cmdline[i] = ' ';
            }
        }
    } else {
        info->cmdline[0] = '\0';
    }
    
    close(fd);
    return 0;
}

static int read_proc_exe(pid_t pid, linx_process_info_t *info)
{
    char path[PROC_PATH_MAX_LEN];
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    len = readlink(path, info->exe, PROC_PATH_MAX_LEN - 1);
    if (len > 0) {
        info->exe[len] = '\0';
    } else {
        info->exe[0] = '\0';
    }
    
    return 0;
}

static int read_proc_cwd(pid_t pid, linx_process_info_t *info)
{
    char path[PROC_PATH_MAX_LEN];
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/cwd", pid);
    len = readlink(path, info->cwd, PROC_PATH_MAX_LEN - 1);
    if (len > 0) {
        info->cwd[len] = '\0';
    } else {
        info->cwd[0] = '\0';
    }
    
    return 0;
}

static int is_process_alive(pid_t pid)
{
    return (kill(pid, 0) == 0 || errno == EPERM);
}

static linx_process_info_t *create_process_info(pid_t pid)
{
    linx_process_info_t *info = calloc(1, sizeof(linx_process_info_t));
    if (!info) {
        return NULL;
    }

    info->pid = pid;
    info->create_time = time(NULL);
    info->update_time = info->create_time;
    info->is_alive = 1;
    
    /* 读取进程信息 */
    if (read_proc_stat(pid, info) < 0 ||
        read_proc_status(pid, info) < 0) {
        free(info);
        return NULL;
    }

    /* 尝试读取其他信息，失败不影响创建 */
    read_proc_cmdline(pid, info);
    read_proc_exe(pid, info);
    read_proc_cwd(pid, info);
    
    return info;
}

static void free_process_info(linx_process_info_t *info)
{
    if (info) {
        free(info);
    }
}

static void *update_process_task(void *arg, int *should_stop)
{
    pid_t pid = *(pid_t *)arg;
    linx_process_info_t *info, *old_info;

    free(arg);

    if (*should_stop) {
        return NULL;
    }

    info = create_process_info(pid);
    if (!info) {
        // 如果无法创建进程信息，可能是进程已退出
        // 检查是否有已存在的缓存项，如果有则标记为已退出
        pthread_rwlock_wrlock(&g_process_cache->lock);
        HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
        if (old_info && old_info->is_alive) {
            old_info->is_alive = 0;
            old_info->exit_time = time(NULL);
        }
        pthread_rwlock_unlock(&g_process_cache->lock);
        return NULL;
    }

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    if (old_info) {
        info->create_time = old_info->create_time;

        if (!old_info->is_alive && old_info->exit_time > 0) {
            info->exit_time = old_info->exit_time;
            info->is_alive = 0;
        }

        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }

    HASH_ADD_INT(g_process_cache->hash_table, pid, info);

    pthread_rwlock_unlock(&g_process_cache->lock);

    return NULL;
}

/**
 * 解析inotify事件中的进程ID
 */
static pid_t extract_pid_from_name(const char *name)
{
    if (!name || !isdigit(name[0])) {
        return -1;
    }
    
    pid_t pid = atoi(name);
    return (pid > 0) ? pid : -1;
}

/**
 * 处理进程创建事件
 */
static void handle_process_create(pid_t pid)
{
    pid_t *pid_arg = malloc(sizeof(pid_t));
    if (pid_arg) {
        *pid_arg = pid;
        if (linx_thread_pool_add_task(g_process_cache->thread_pool, update_process_task, pid_arg) < 0) {
            free(pid_arg);
        }
    }
}

/**
 * 处理进程删除事件
 */
static void handle_process_delete(pid_t pid)
{
    linx_process_info_t *info;
    
    pthread_rwlock_wrlock(&g_process_cache->lock);
    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    if (info && info->is_alive) {
        info->is_alive = 0;
        info->exit_time = time(NULL);
        info->state = LINX_PROCESS_STATE_EXITED;
    }
    pthread_rwlock_unlock(&g_process_cache->lock);
}

/**
 * 基于inotify的/proc文件系统监控线程
 * 实时监控进程的创建和删除，相比轮询方式更加高效
 */
static void *inotify_monitor_thread_func(void *arg, int *should_stop)
{
    (void)arg;
    char buffer[LINX_PROCESS_CACHE_INOTIFY_BUFFER_SIZE];
    ssize_t length;
    struct inotify_event *event;
    char *ptr;
    pid_t pid;
    fd_set read_fds;
    struct timeval timeout;
    int ret;

    while (g_process_cache->running && !*should_stop) {
        // 使用select进行超时控制，避免阻塞太久
        FD_ZERO(&read_fds);
        FD_SET(g_process_cache->inotify_fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        ret = select(g_process_cache->inotify_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("ERROR: select failed: %s\n", strerror(errno));
            break;
        } else if (ret == 0) {
            // 超时，继续下一轮循环
            continue;
        }

        if (!FD_ISSET(g_process_cache->inotify_fd, &read_fds)) {
            continue;
        }

        length = read(g_process_cache->inotify_fd, buffer, sizeof(buffer));
        if (length < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            printf("ERROR: read inotify events failed: %s\n", strerror(errno));
            break;
        }

        // 处理inotify事件
        ptr = buffer;
        while (ptr < buffer + length) {
            event = (struct inotify_event *)ptr;
            
            if (event->len > 0) {
                pid = extract_pid_from_name(event->name);
                if (pid > 0) {
                    if (event->mask & IN_CREATE) {
                        // 进程创建事件
                        handle_process_create(pid);
                    } else if (event->mask & IN_DELETE) {
                        // 进程删除事件
                        handle_process_delete(pid);
                    }
                }
            }
            
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    return NULL;
}

/**
 * 轮询模式的监控线程（作为备用方案）
 */
static void *polling_monitor_thread_func(void *arg, int *should_stop)
{
    (void)arg;
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid, *pid_arg;
    linx_process_info_t *info, *tmp;

    while (g_process_cache->running && !*should_stop) {
        proc_dir = opendir("/proc");
        if (proc_dir == NULL) {
            usleep(1000 * 100);
            continue;
        }

        while ((entry = readdir(proc_dir)) != NULL) {
            if (!isdigit(entry->d_name[0])) {
                continue;
            }

            pid = atoi(entry->d_name);
            if (pid <= 0) {
                continue;
            }

            pid_arg = malloc(sizeof(pid_t));
            if (pid_arg) {
                *pid_arg = pid;
                if (linx_thread_pool_add_task(g_process_cache->thread_pool, update_process_task, pid_arg) < 0) {
                    free(pid_arg);
                }
            }
        }

        closedir(proc_dir);

        pthread_rwlock_wrlock(&g_process_cache->lock);

        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            if (info->is_alive && !is_process_alive(info->pid)) {
                info->is_alive = 0;
                info->exit_time = time(NULL);
                info->state = LINX_PROCESS_STATE_EXITED;
            }
        }

        pthread_rwlock_unlock(&g_process_cache->lock);

        usleep(1000 * 100);
    }

    return NULL;
}

static void *cleaner_thread_func(void *arg, int *should_stop)
{
    (void)arg;
    time_t now;
    linx_process_info_t *info, *tmp;
    
    while (g_process_cache->running && !*should_stop) {
        sleep(LINX_PROCESS_CACHE_EXPIRE_TIME / 2);
        
        now = time(NULL);
        
        pthread_rwlock_wrlock(&g_process_cache->lock);

        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            /* 清理已退出且超过保留时间的进程 */
            if (!info->is_alive && info->exit_time > 0) {
                time_t retain_time;
                // 对于短生命周期进程（存活时间很短），使用较短的保留时间
                // 这样可以在保证规则匹配的同时，避免缓存占用过多内存
                if ((info->exit_time - info->create_time) <= 5) {
                    retain_time = LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME;
                } else {
                    retain_time = LINX_PROCESS_CACHE_EXPIRE_TIME;
                }
                
                if ((now - info->exit_time) > retain_time) {
                    HASH_DEL(g_process_cache->hash_table, info);
                    free_process_info(info);
                }
            }
        }

        pthread_rwlock_unlock(&g_process_cache->lock);
    }
    
    return NULL;
}

/**
 * 初始化inotify监控
 */
static int init_inotify_monitor(void)
{
    // 创建inotify实例
    g_process_cache->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (g_process_cache->inotify_fd < 0) {
        printf("ERROR: Failed to initialize inotify: %s\n", strerror(errno));
        return -1;
    }

    // 监控/proc目录
    g_process_cache->proc_watch_fd = inotify_add_watch(g_process_cache->inotify_fd, 
                                                       "/proc", 
                                                       IN_CREATE | IN_DELETE);
    if (g_process_cache->proc_watch_fd < 0) {
        printf("ERROR: Failed to add /proc to inotify watch: %s\n", strerror(errno));
        close(g_process_cache->inotify_fd);
        g_process_cache->inotify_fd = -1;
        return -1;
    }

    printf("INFO: Successfully initialized inotify monitor for /proc filesystem\n");
    return 0;
}

/**
 * 清理inotify监控
 */
static void cleanup_inotify_monitor(void)
{
    if (g_process_cache->proc_watch_fd >= 0) {
        inotify_rm_watch(g_process_cache->inotify_fd, g_process_cache->proc_watch_fd);
        g_process_cache->proc_watch_fd = -1;
    }
    
    if (g_process_cache->inotify_fd >= 0) {
        close(g_process_cache->inotify_fd);
        g_process_cache->inotify_fd = -1;
    }
}

int linx_process_cache_init(void)
{
    void *(*monitor_func)(void *, int *) = NULL;
    
    if (g_process_cache) {
        return 0;
    }

    if (linx_process_cache_bind_field()) {
        return -1;
    }

    g_process_cache = calloc(1, sizeof(linx_process_cache_t));
    if (!g_process_cache) {
        return -1;
    }

    // 初始化inotify相关字段
    g_process_cache->inotify_fd = -1;
    g_process_cache->proc_watch_fd = -1;

    if (pthread_rwlock_init(&g_process_cache->lock, NULL) != 0) {
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    g_process_cache->thread_pool = linx_thread_pool_create(LINX_PROCESS_CACHE_THREAD_NUM);
    if (!g_process_cache->thread_pool) {
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    g_process_cache->running = 1;

         // 尝试初始化inotify监控，如果失败则使用轮询模式
    if (init_inotify_monitor() == 0) {
        monitor_func = inotify_monitor_thread_func;
        printf("INFO: Using inotify-based process monitoring (real-time)\n");
    } else {
        monitor_func = polling_monitor_thread_func;
        printf("WARN: Fallback to polling-based process monitoring\n");
    }

    if (linx_thread_pool_add_task(g_process_cache->thread_pool, monitor_func, NULL) < 0) {
        cleanup_inotify_monitor();
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    if (linx_thread_pool_add_task(g_process_cache->thread_pool, cleaner_thread_func, NULL) < 0) {
        g_process_cache->running = 0;
        cleanup_inotify_monitor();
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    return 0;
}

void linx_process_cache_deinit(void)
{
    linx_process_info_t *info, *tmp;
    if (!g_process_cache) {
        return;
    }

    g_process_cache->running = 0;

    // 清理inotify监控
    cleanup_inotify_monitor();

    linx_thread_pool_destroy(g_process_cache->thread_pool, 1);

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
        HASH_DEL(g_process_cache->hash_table, info);
        free_process_info(info);
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    pthread_rwlock_destroy(&g_process_cache->lock);

    free(g_process_cache);
    g_process_cache = NULL;
}

linx_process_info_t *linx_process_cache_get(pid_t pid)
{
    linx_process_info_t *info = NULL;

    if (!g_process_cache) {
        return NULL;
    }

    pthread_rwlock_rdlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    if (!info) {
        info = create_process_info(pid);
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    return info;
}

int linx_process_cache_get_all(linx_process_info_t **list, int *count)
{
    linx_process_info_t *info;
    int i = 0;

    if (!g_process_cache || !list || !count) {
        return -1;
    }

    pthread_rwlock_rdlock(&g_process_cache->lock);

    *count = HASH_COUNT(g_process_cache->hash_table);
    if (*count == 0) {
        *list = NULL;
        pthread_rwlock_unlock(&g_process_cache->lock);
        return 0;
    }

    *list = calloc(*count, sizeof(linx_process_info_t));
    if (!*list) {
        pthread_rwlock_unlock(&g_process_cache->lock);
        return -1;
    }

    for (info = g_process_cache->hash_table; info != NULL; info = info->hh.next) {
        memcpy(&(*list)[i], info, sizeof(linx_process_info_t));
        memset(&(*list)[i].hh, 0, sizeof(UT_hash_handle));
        i++;
    }

    pthread_rwlock_unlock(&g_process_cache->lock);
    return 0;
}

int linx_process_cache_update_async(pid_t pid)
{
    pid_t *pid_arg;

    if (!g_process_cache) {
        return -1;
    }

    pid_arg = malloc(sizeof(pid_t));
    if (!pid_arg) {
        return -1;
    }

    *pid_arg = pid;
    return linx_thread_pool_add_task(g_process_cache->thread_pool, update_process_task, pid_arg);
}

int linx_process_cache_update_sync(pid_t pid)
{
    pid_t *pid_arg;
    int tmp = 0;

    if (!g_process_cache) {
        return -1;
    }

    pid_arg = malloc(sizeof(pid_t));
    if (!pid_arg) {
        return -1;
    }

    *pid_arg = pid;
    update_process_task((void *)pid_arg, &tmp);

    return 0;
}

int linx_process_cache_delete(pid_t pid)
{
    linx_process_info_t *info;

    if (!g_process_cache) {
        return -1;
    }

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    if (info) {
        HASH_DEL(g_process_cache->hash_table, info);
        free_process_info(info);
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    return 0;
}

int linx_process_cache_cleanup(void)
{
    time_t now;
    linx_process_info_t *info, *tmp;
    int cleaned = 0;

    if (!g_process_cache) {
        return -1;
    }

    now = time(NULL);

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_ITER(hh, g_process_cache->hash_table, info, tmp) { 
        if (!info->is_alive && info->exit_time > 0 &&
            (now - info->exit_time) > LINX_PROCESS_CACHE_EXPIRE_TIME)
        {
            HASH_DEL(g_process_cache->hash_table, info);
            free_process_info(info);
            cleaned++;
        }
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    return cleaned;
}

int linx_process_cache_create_from_event(pid_t pid, const char *comm, const char *cmdline)
{
    linx_process_info_t *info, *old_info;
    
    if (!g_process_cache) {
        return -1;
    }
    
    // 先检查是否已经存在
    pthread_rwlock_rdlock(&g_process_cache->lock);
    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    if (old_info) {
        // 已存在，只需要标记为已退出（因为是EXIT事件触发的）
        pthread_rwlock_wrlock(&g_process_cache->lock);
        if (old_info->is_alive) {
            old_info->is_alive = 0;
            old_info->exit_time = time(NULL);
            old_info->state = LINX_PROCESS_STATE_EXITED;
        }
        pthread_rwlock_unlock(&g_process_cache->lock);
        return 0;
    }
    
    // 创建新的进程信息项（基于事件数据）
    info = calloc(1, sizeof(linx_process_info_t));
    if (!info) {
        return -1;
    }
    
    // 填充基本信息
    info->pid = pid;
    info->create_time = time(NULL);
    info->update_time = info->create_time;
    info->exit_time = info->create_time; // 进程已退出
    info->is_alive = 0; // 标记为已退出
    info->state = LINX_PROCESS_STATE_EXITED;
    
    // 从事件数据中获取进程名和命令行
    if (comm) {
        strncpy(info->name, comm, sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';
        strncpy(info->comm, comm, sizeof(info->comm) - 1);
        info->comm[sizeof(info->comm) - 1] = '\0';
    }
    
    if (cmdline) {
        strncpy(info->cmdline, cmdline, sizeof(info->cmdline) - 1);
        info->cmdline[sizeof(info->cmdline) - 1] = '\0';
    }
    
    // 其他字段保持默认值（0）
    
    // 添加到缓存
    pthread_rwlock_wrlock(&g_process_cache->lock);
    HASH_ADD_INT(g_process_cache->hash_table, pid, info);
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    printf("INFO: Created process cache from event data for short-lived process %d (%s)\n", 
           pid, comm ? comm : "unknown");
    
    return 0;
}

void linx_process_cache_stats(int *total, int *alive, int *expired)
{
    int t = 0, a = 0, e = 0;
    linx_process_info_t *info;

    if (!g_process_cache) {
        if (total) {
            *total = 0;
        }

        if (alive) {
            *alive = 0;
        }

        if (expired) {
            *expired = 0;
        }
        return;
    }

    pthread_rwlock_rdlock(&g_process_cache->lock);

    for (info = g_process_cache->hash_table; info != NULL; info = info->hh.next) {
        t++;
        if (info->is_alive) {
            a++;
        } else {
            e++;
        }
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    if (total) {
        *total = t;
    }

    if (alive) {
        *alive = a;
    }

    if (expired) {
        *expired = e;
    }
}

int linx_process_cache_get_monitor_status(char *status_buf, size_t buf_size)
{
    if (!status_buf || buf_size == 0) {
        return -1;
    }
    
    if (!g_process_cache) {
        snprintf(status_buf, buf_size, "Process cache not initialized");
        return -1;
    }
    
    const char *monitor_type = (g_process_cache->inotify_fd >= 0) ? 
                               "inotify-based (real-time)" : 
                               "polling-based (fallback)";
    
    int total = 0, alive = 0, expired = 0;
    linx_process_cache_stats(&total, &alive, &expired);
    
    snprintf(status_buf, buf_size, 
             "Monitor Type: %s, Total Processes: %d, Alive: %d, Exited: %d", 
             monitor_type, total, alive, expired);
    
    return 0;
}
