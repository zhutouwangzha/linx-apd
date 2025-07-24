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
#include <sys/time.h>
#include <stdatomic.h>

#include "linx_process_cache.h"
#include "linx_hash_map.h"

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
            sscanf(line, "RssShmem:\t%lu klinx_process_cache_updateB", &info->shared);
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

/* 快速检查PID是否存在，避免系统调用开销 */
static int quick_pid_check(pid_t pid)
{
    char path[64];
    struct stat st;
    
    snprintf(path, sizeof(path), "/proc/%d", pid);
    return (stat(path, &st) == 0);
}

/* 高效的PID范围扫描 */
static int scan_pid_range(int start_pid, int end_pid, pid_t *found_pids, int max_pids)
{
    int count = 0;
    char path[64];
    struct stat st;
    
    for (int pid = start_pid; pid <= end_pid && count < max_pids; pid++) {
        snprintf(path, sizeof(path), "/proc/%d", pid);
        if (stat(path, &st) == 0) {
            found_pids[count++] = pid;
        }
    }
    
    return count;
}

/* 获取系统中的最大PID值 */
static int get_max_pid(void)
{
    FILE *fp;
    int max_pid = 32768; /* 默认值 */
    
    fp = fopen("/proc/sys/kernel/pid_max", "r");
    if (fp) {
        fscanf(fp, "%d", &max_pid);
        fclose(fp);
    }
    
    return max_pid;
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
    
    /* 优先读取关键信息 */
    if (read_proc_stat(pid, info) < 0) {
        free(info);
        return NULL;
    }

    /* 读取状态信息，失败时使用默认值 */
    if (read_proc_status(pid, info) < 0) {
        /* 使用默认值，不失败 */
        info->uid = 0;
        info->gid = 0;
    }

    /* 并行读取其他信息，失败不影响创建 */
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

/* 快速扫描任务 */
static void *fast_scan_task(void *arg, int *should_stop)
{
    fast_scan_task_t *task = (fast_scan_task_t *)arg;
    pid_t found_pids[LINX_PROCESS_CACHE_BATCH_SIZE];
    int found_count;
    pid_t *pid_arg;
    time_t now;
    
    if (!task) {
        return NULL;
    }
    
    found_count = scan_pid_range(task->start_pid, task->end_pid, 
                                found_pids, LINX_PROCESS_CACHE_BATCH_SIZE);
    
    atomic_fetch_add(&g_process_cache->total_scanned, found_count);
    
    now = time(NULL);
    
    for (int i = 0; i < found_count && !*should_stop; i++) {
        pid_t pid = found_pids[i];
        
        /* 检查是否已经缓存 */
        pthread_rwlock_rdlock(&g_process_cache->lock);
        linx_process_info_t *existing;
        HASH_FIND_INT(g_process_cache->hash_table, &pid, existing);
        pthread_rwlock_unlock(&g_process_cache->lock);
        
        if (!existing) {
            /* 新进程，异步更新 */
            pid_arg = malloc(sizeof(pid_t));
            if (pid_arg) {
                *pid_arg = pid;
                linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                        update_process_task, pid_arg);
            }
        } else if (existing->is_alive && 
                  (now - existing->update_time) > LINX_PROCESS_CACHE_UPDATE_INTERVAL) {
            /* 已存在但需要更新 */
            pid_arg = malloc(sizeof(pid_t));
            if (pid_arg) {
                *pid_arg = pid;
                linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                        update_process_task, pid_arg);
            }
        }
    }
    
    free(task);
    return NULL;
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
            info->is_alive = 0;
        }

        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }

    HASH_ADD_INT(g_process_cache->hash_table, pid, info);

    pthread_rwlock_unlock(&g_process_cache->lock);

    return NULL;
}

/* 高频扫描线程 */
static void *high_freq_scan_thread(void *arg, int *should_stop)
{
    int thread_id = *(int *)arg;
    int max_pid = get_max_pid();
    int pid_range = max_pid / LINX_PROCESS_CACHE_FAST_SCAN_THREADS;
    int start_pid = thread_id * pid_range + 1;
    int end_pid = (thread_id + 1) * pid_range;
    
    if (thread_id == LINX_PROCESS_CACHE_FAST_SCAN_THREADS - 1) {
        end_pid = max_pid; /* 最后一个线程处理剩余范围 */
    }
    
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = LINX_PROCESS_CACHE_HIGH_FREQ_INTERVAL_MS * 1000000L;
    
    free(arg);
    
    while (atomic_load(&g_process_cache->running) && !*should_stop) {
        /* 检查是否在高频模式 */
        if (!atomic_load(&g_process_cache->high_freq_mode)) {
            sleep(1);
            continue;
        }
        
        fast_scan_task_t *task = malloc(sizeof(fast_scan_task_t));
        if (task) {
            task->start_pid = start_pid;
            task->end_pid = end_pid;
            task->thread_id = thread_id;
            
            linx_thread_pool_add_task(g_process_cache->fast_scan_pool, 
                                    fast_scan_task, task);
        }
        
        atomic_fetch_add(&g_process_cache->scan_cycles, 1);
        nanosleep(&sleep_time, NULL);
    }
    
    return NULL;
}

static void *monitor_thread_func(void *arg, int *should_stop)
{
    (void)arg;
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid, *pid_arg;
    linx_process_info_t *info, *tmp;
    time_t now, last_check = 0;

    while (atomic_load(&g_process_cache->running) && !*should_stop) {
        now = time(NULL);
        
        /* 检查是否需要退出高频模式 */
        if (atomic_load(&g_process_cache->high_freq_mode)) {
            if (now - g_process_cache->high_freq_start_time > 
                LINX_PROCESS_CACHE_HIGH_FREQ_DURATION) {
                atomic_store(&g_process_cache->high_freq_mode, 0);
                printf("退出高频扫描模式\n");
            }
        }
        
        /* 低频率的完整扫描作为备份 */
        if (now - last_check >= LINX_PROCESS_CACHE_UPDATE_INTERVAL) {
            proc_dir = opendir("/proc");
            if (proc_dir != NULL) {
                while ((entry = readdir(proc_dir)) != NULL) {
                    if (!isdigit(entry->d_name[0])) {
                        continue;
                    }

                    pid = atoi(entry->d_name);
                    if (pid <= 0) {
                        continue;
                    }

                    /* 检查是否已缓存 */
                    pthread_rwlock_rdlock(&g_process_cache->lock);
                    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
                    pthread_rwlock_unlock(&g_process_cache->lock);
                    
                    if (!info) {
                        pid_arg = malloc(sizeof(pid_t));
                        if (pid_arg) {
                            *pid_arg = pid;
                            linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                                    update_process_task, pid_arg);
                        }
                    }
                }
                closedir(proc_dir);
            }
            last_check = now;
        }

        /* 检查已缓存进程的存活状态 */
        pthread_rwlock_wrlock(&g_process_cache->lock);

        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            if (info->is_alive && !quick_pid_check(info->pid)) {
                info->is_alive = 0;
                info->exit_time = now;
                info->state = LINX_PROCESS_STATE_EXITED;
                
                /* 统计短暂进程 */
                if (info->exit_time - info->create_time < LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME) {
                    atomic_fetch_add(&g_process_cache->short_lived_cached, 1);
                }
            }
        }

        pthread_rwlock_unlock(&g_process_cache->lock);

        usleep(100000); /* 100ms */
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
                time_t retention_time;
                
                /* 短暂进程（生命周期小于设定时间）使用较短的保留时间 */
                if (info->exit_time - info->create_time < LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME) {
                    retention_time = LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME;
                } else {
                    retention_time = LINX_PROCESS_CACHE_EXPIRE_TIME;
                }
                
                if ((now - info->exit_time) > retention_time) {
                    HASH_DEL(g_process_cache->hash_table, info);
                    free_process_info(info);
                }
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

    /* 初始化原子变量 */
    atomic_init(&g_process_cache->running, 1);
    atomic_init(&g_process_cache->total_scanned, 0);
    atomic_init(&g_process_cache->short_lived_cached, 0);
    atomic_init(&g_process_cache->scan_cycles, 0);

    /* 创建线程池 */
    g_process_cache->thread_pool = linx_thread_pool_create(LINX_PROCESS_CACHE_THREAD_NUM);
    if (!g_process_cache->thread_pool) {
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    g_process_cache->fast_scan_pool = linx_thread_pool_create(LINX_PROCESS_CACHE_FAST_SCAN_THREADS * 2);
    if (!g_process_cache->fast_scan_pool) {
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    /* 启动监控线程 */
    if (linx_thread_pool_add_task(g_process_cache->thread_pool, monitor_thread_func, NULL) < 0) {
        linx_thread_pool_destroy(g_process_cache->fast_scan_pool, 0);
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

    /* 启动清理线程 */
    if (linx_thread_pool_add_task(g_process_cache->thread_pool, cleaner_thread_func, NULL) < 0) {
        atomic_store(&g_process_cache->running, 0);
        linx_thread_pool_destroy(g_process_cache->fast_scan_pool, 0);
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }

#if LINX_PROCESS_CACHE_HIGH_FREQ_ENABLED
    /* 启用高频扫描模式 */
    atomic_init(&g_process_cache->high_freq_mode, 1);
    g_process_cache->high_freq_start_time = time(NULL);
    
    /* 创建高频扫描线程 */
    g_process_cache->fast_scan_threads = malloc(sizeof(pthread_t) * LINX_PROCESS_CACHE_FAST_SCAN_THREADS);
    if (!g_process_cache->fast_scan_threads) {
        atomic_store(&g_process_cache->running, 0);
        linx_thread_pool_destroy(g_process_cache->fast_scan_pool, 0);
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }
    
    for (int i = 0; i < LINX_PROCESS_CACHE_FAST_SCAN_THREADS; i++) {
        int *thread_id = malloc(sizeof(int));
        if (thread_id) {
            *thread_id = i;
            if (linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                        high_freq_scan_thread, thread_id) < 0) {
                free(thread_id);
            }
        }
    }
    
    printf("启用高频扫描模式，持续 %d 秒，扫描间隔 %d ms\n", 
           LINX_PROCESS_CACHE_HIGH_FREQ_DURATION, 
           LINX_PROCESS_CACHE_HIGH_FREQ_INTERVAL_MS);
#else
    atomic_init(&g_process_cache->high_freq_mode, 0);
#endif

    return 0;
}

void linx_process_cache_deinit(void)
{
    linx_process_info_t *info, *tmp;
    if (!g_process_cache) {
        return;
    }

    atomic_store(&g_process_cache->running, 0);
    atomic_store(&g_process_cache->high_freq_mode, 0);

    linx_thread_pool_destroy(g_process_cache->thread_pool, 1);
    linx_thread_pool_destroy(g_process_cache->fast_scan_pool, 1);

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
        HASH_DEL(g_process_cache->hash_table, info);
        free_process_info(info);
    }

    pthread_rwlock_unlock(&g_process_cache->lock);

    pthread_rwlock_destroy(&g_process_cache->lock);

    if (g_process_cache->fast_scan_threads) {
        free(g_process_cache->fast_scan_threads);
    }

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

int linx_process_cache_update(pid_t pid)
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

/* 新增高性能接口实现 */
int linx_process_cache_force_update(pid_t pid)
{
    linx_process_info_t *info, *old_info;

    if (!g_process_cache || pid <= 0) {
        return -1;
    }

    info = create_process_info(pid);
    if (!info) {
        return -1;
    }

    pthread_rwlock_wrlock(&g_process_cache->lock);

    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    if (old_info) {
        info->create_time = old_info->create_time;
        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }

    HASH_ADD_INT(g_process_cache->hash_table, pid, info);

    pthread_rwlock_unlock(&g_process_cache->lock);

    return 0;
}

int linx_process_cache_batch_update(pid_t *pids, int count)
{
    if (!g_process_cache || !pids || count <= 0) {
        return -1;
    }

    for (int i = 0; i < count; i++) {
        pid_t *pid_arg = malloc(sizeof(pid_t));
        if (pid_arg) {
            *pid_arg = pids[i];
            linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                    update_process_task, pid_arg);
        }
    }

    return 0;
}

void linx_process_cache_get_detailed_stats(int *total, int *alive, int *expired, 
                                         long *scanned, long *short_lived, long *cycles)
{
    int t = 0, a = 0, e = 0;
    linx_process_info_t *info;

    if (!g_process_cache) {
        if (total) *total = 0;
        if (alive) *alive = 0;
        if (expired) *expired = 0;
        if (scanned) *scanned = 0;
        if (short_lived) *short_lived = 0;
        if (cycles) *cycles = 0;
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

    if (total) *total = t;
    if (alive) *alive = a;
    if (expired) *expired = e;
    if (scanned) *scanned = atomic_load(&g_process_cache->total_scanned);
    if (short_lived) *short_lived = atomic_load(&g_process_cache->short_lived_cached);
    if (cycles) *cycles = atomic_load(&g_process_cache->scan_cycles);
}

int linx_process_cache_set_high_freq_mode(int enabled)
{
    if (!g_process_cache) {
        return -1;
    }

    if (enabled) {
        atomic_store(&g_process_cache->high_freq_mode, 1);
        g_process_cache->high_freq_start_time = time(NULL);
        printf("启用高频扫描模式\n");
    } else {
        atomic_store(&g_process_cache->high_freq_mode, 0);
        printf("禁用高频扫描模式\n");
    }

    return 0;
}
