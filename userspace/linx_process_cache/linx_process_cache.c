#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "linx_process_cache.h"
#include "linx_hash_map.h"

static linx_process_cache_t *g_process_cache = NULL;

static int linx_process_cache_bind_field(void)
{
    BEGIN_FIELD_MAPPINGS(proc)
        FIELD_MAP(linx_process_info_t, pid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, ppid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, pgid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, sid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, uid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, gid, LINX_FIELD_TYPE_INT32)
        FIELD_MAP(linx_process_info_t, name, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, comm, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, cmdline, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, exe, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(linx_process_info_t, cwd, LINX_FIELD_TYPE_CHARBUF)
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
    info->is_alive = true;
    info->is_rich = false;
    
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
        return NULL;
    }

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    if (old_info) {
        info->create_time = old_info->create_time;

        if (!old_info->is_alive && old_info->exit_time > 0) {
            info->exit_time = old_info->exit_time;
            info->is_alive = false;
        }

        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }

    HASH_ADD_INT(g_process_cache->hash_table, pid, info);

    pthread_rwlock_unlock(&g_process_cache->lock);

    return NULL;
}

static void *monitor_thread_func(void *arg, int *should_stop)
{
    (void)arg;
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid, *pid_arg;
    linx_process_info_t *info, *tmp;

    while (g_process_cache->running && !*should_stop) {
        proc_dir = opendir("/proc");
        if (proc_dir == NULL) {
            sleep(LINX_PROCESS_CACHE_UPDATE_INTERVAL);
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
                linx_thread_pool_add_task(g_process_cache->thread_pool, update_process_task, pid_arg);
            }
        }

        closedir(proc_dir);

        pthread_rwlock_wrlock(&g_process_cache->lock);

        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            if (!info->is_rich && info->is_alive && !is_process_alive(info->pid)) {
                info->is_alive = false;
                info->exit_time = time(NULL);
                info->state = LINX_PROCESS_STATE_EXITED;
            }
        }

        pthread_rwlock_unlock(&g_process_cache->lock);

        sleep(LINX_PROCESS_CACHE_UPDATE_INTERVAL);
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
            if ((info->is_rich && 
                 (now - info->start_time) > LINX_PROCESS_CACHE_EXPIRE_TIME) ||
                (!info->is_alive && info->exit_time > 0 &&
                (now - info->exit_time) > LINX_PROCESS_CACHE_EXPIRE_TIME)) 
            {
                HASH_DEL(g_process_cache->hash_table, info);
                free_process_info(info);
            }
        }

        pthread_rwlock_unlock(&g_process_cache->lock);
    }
    
    return NULL;
}

int linx_process_cache_init(void)
{
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

    if (linx_thread_pool_add_task(g_process_cache->thread_pool, monitor_thread_func, NULL) < 0) {
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    if (linx_thread_pool_add_task(g_process_cache->thread_pool, cleaner_thread_func, NULL) < 0) {
        g_process_cache->running = 0;
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
    pthread_rwlock_unlock(&g_process_cache->lock);

    if (!info) {
        // 当找不到时，创建新的进程信息并添加到哈希表
        info = create_process_info(pid);
        if (info) {
            pthread_rwlock_wrlock(&g_process_cache->lock);
            
            // 再次检查，防止并发情况下重复添加
            linx_process_info_t *existing_info = NULL;
            HASH_FIND_INT(g_process_cache->hash_table, &pid, existing_info);
            if (!existing_info) {
                HASH_ADD_INT(g_process_cache->hash_table, pid, info);
            } else {
                // 如果在并发情况下其他线程已经添加了，释放当前创建的并返回已存在的
                free_process_info(info);
                info = existing_info;
            }
            
            pthread_rwlock_unlock(&g_process_cache->lock);
        }
    }

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

int linx_process_cache_update(linx_process_info_t *info)
{
    linx_process_info_t *old_info;
    pid_t pid;

    if (!info) {
        return -1;
    }

    pid = info->pid;

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    if (old_info) {
        info->create_time = old_info->create_time;

        if (!old_info->is_alive && old_info->exit_time > 0) {
            info->exit_time = old_info->exit_time;
            info->is_alive = false;
        }

        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }

    HASH_ADD_INT(g_process_cache->hash_table, pid, info);

    pthread_rwlock_unlock(&g_process_cache->lock);

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

// 新增函数：查找或创建进程缓存
linx_process_info_t *linx_process_cache_get_or_create(pid_t pid)
{
    linx_process_info_t *info = NULL;

    if (!g_process_cache) {
        return NULL;
    }

    pthread_rwlock_rdlock(&g_process_cache->lock);
    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    pthread_rwlock_unlock(&g_process_cache->lock);

    if (!info) {
        // 创建新的进程信息并添加到哈希表
        info = create_process_info(pid);
        if (info) {
            pthread_rwlock_wrlock(&g_process_cache->lock);
            
            // 再次检查，防止并发情况下重复添加
            linx_process_info_t *existing_info = NULL;
            HASH_FIND_INT(g_process_cache->hash_table, &pid, existing_info);
            if (!existing_info) {
                HASH_ADD_INT(g_process_cache->hash_table, pid, info);
            } else {
                // 如果在并发情况下其他线程已经添加了，释放当前创建的并返回已存在的
                free_process_info(info);
                info = existing_info;
            }
            
            pthread_rwlock_unlock(&g_process_cache->lock);
        }
    }

    return info;
}
