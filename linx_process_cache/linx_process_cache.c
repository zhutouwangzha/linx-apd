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
#include <pthread.h>
#include <signal.h>
#include "linx_process_cache.h"

/* 全局进程缓存管理器 */
static process_cache_t *g_process_cache = NULL;

/* 内部函数声明 */
static int read_proc_stat(pid_t pid, process_info_t *info);
static int read_proc_status(pid_t pid, process_info_t *info);
static int read_proc_cmdline(pid_t pid, process_info_t *info);
static int read_proc_exe(pid_t pid, process_info_t *info);
static int read_proc_cwd(pid_t pid, process_info_t *info);
static int is_process_alive(pid_t pid);
static process_info_t *create_process_info(pid_t pid);
static void free_process_info(process_info_t *info);
static void *monitor_thread_func(void *arg);
static void *cleaner_thread_func(void *arg);
static void *update_process_task(void *arg, int *should_stop);

/**
 * @brief 读取/proc/[pid]/stat文件获取进程状态信息
 */
static int read_proc_stat(pid_t pid, process_info_t *info)
{
    char path[PROC_PATH_MAX];
    char buffer[4096];
    FILE *fp;
    char state;
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
    
    /* 解析stat文件，格式复杂，需要处理进程名中可能包含的空格和括号 */
    char *start = strchr(buffer, '(');
    char *end = strrchr(buffer, ')');
    if (!start || !end || end <= start) {
        fclose(fp);
        return -1;
    }
    
    /* 复制进程名 */
    size_t name_len = end - start - 1;
    if (name_len >= PROC_COMM_MAX) {
        name_len = PROC_COMM_MAX - 1;
    }
    strncpy(info->comm, start + 1, name_len);
    info->comm[name_len] = '\0';
    
    /* 解析其余字段 */
    ret = sscanf(end + 2, " %c %d %d %d %*d %*d %*u %*u %*u %*u %*u %lu %lu "
                 "%*d %*d %d %d %*d %*d %lu %lu %ld",
                 &state, &info->ppid, &info->pgid, &info->sid,
                 &info->utime, &info->stime,
                 &info->priority, &info->nice,
                 &info->start_time, &info->vsize, &info->rss);
    
    if (ret < 10) {
        fclose(fp);
        return -1;
    }
    
    /* 转换进程状态 */
    switch (state) {
        case 'R': info->state = PROCESS_STATE_RUNNING; break;
        case 'S': info->state = PROCESS_STATE_SLEEPING; break;
        case 'Z': info->state = PROCESS_STATE_ZOMBIE; break;
        case 'T': info->state = PROCESS_STATE_STOPPED; break;
        default: info->state = PROCESS_STATE_UNKNOWN; break;
    }
    
    /* RSS是以页为单位的，转换为字节 */
    info->rss *= getpagesize();
    
    fclose(fp);
    return 0;
}

/**
 * @brief 读取/proc/[pid]/status文件获取进程详细信息
 */
static int read_proc_status(pid_t pid, process_info_t *info)
{
    char path[PROC_PATH_MAX];
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

/**
 * @brief 读取/proc/[pid]/cmdline文件获取进程命令行
 */
static int read_proc_cmdline(pid_t pid, process_info_t *info)
{
    char path[PROC_PATH_MAX];
    int fd;
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    len = read(fd, info->cmdline, PROC_CMDLINE_MAX - 1);
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

/**
 * @brief 读取/proc/[pid]/exe符号链接获取可执行文件路径
 */
static int read_proc_exe(pid_t pid, process_info_t *info)
{
    char path[PROC_PATH_MAX];
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    len = readlink(path, info->exe_path, PROC_PATH_MAX - 1);
    if (len > 0) {
        info->exe_path[len] = '\0';
    } else {
        info->exe_path[0] = '\0';
    }
    
    return 0;
}

/**
 * @brief 读取/proc/[pid]/cwd符号链接获取当前工作目录
 */
static int read_proc_cwd(pid_t pid, process_info_t *info)
{
    char path[PROC_PATH_MAX];
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/cwd", pid);
    len = readlink(path, info->cwd, PROC_PATH_MAX - 1);
    if (len > 0) {
        info->cwd[len] = '\0';
    } else {
        info->cwd[0] = '\0';
    }
    
    return 0;
}

/**
 * @brief 检查进程是否存活
 */
static int is_process_alive(pid_t pid)
{
    return (kill(pid, 0) == 0 || errno == EPERM);
}

/**
 * @brief 创建进程信息结构
 */
static process_info_t *create_process_info(pid_t pid)
{
    process_info_t *info = calloc(1, sizeof(process_info_t));
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

/**
 * @brief 释放进程信息结构
 */
static void free_process_info(process_info_t *info)
{
    if (info) {
        free(info);
    }
}

/**
 * @brief 更新进程信息任务
 */
static void *update_process_task(void *arg, int *should_stop)
{
    pid_t pid = *(pid_t *)arg;
    process_info_t *info, *old_info;
    
    free(arg);
    
    if (*should_stop) {
        return NULL;
    }
    
    /* 创建新的进程信息 */
    info = create_process_info(pid);
    if (!info) {
        return NULL;
    }
    
    /* 更新缓存 */
    pthread_rwlock_wrlock(&g_process_cache->lock);
    
    /* 查找已存在的记录 */
    HASH_FIND_INT(g_process_cache->hash_table, &pid, old_info);
    if (old_info) {
        /* 保留一些历史信息 */
        info->create_time = old_info->create_time;
        if (!old_info->is_alive && old_info->exit_time > 0) {
            info->exit_time = old_info->exit_time;
            info->is_alive = 0;
        }
        /* 替换旧记录 */
        HASH_DEL(g_process_cache->hash_table, old_info);
        free_process_info(old_info);
    }
    
    /* 添加到哈希表 */
    HASH_ADD_INT(g_process_cache->hash_table, pid, info);
    
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    return NULL;
}

/**
 * @brief 监控线程函数 - 定期扫描/proc目录更新缓存
 */
static void *monitor_thread_func(void *arg)
{
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid;
    
    while (g_process_cache->running) {
        proc_dir = opendir("/proc");
        if (!proc_dir) {
            sleep(PROCESS_CACHE_UPDATE_INTERVAL);
            continue;
        }
        
        /* 扫描/proc目录 */
        while ((entry = readdir(proc_dir)) != NULL) {
            /* 检查是否为数字目录名（PID） */
            if (!isdigit(entry->d_name[0])) {
                continue;
            }
            
            pid = atoi(entry->d_name);
            if (pid <= 0) {
                continue;
            }
            
            /* 创建更新任务 */
            pid_t *pid_arg = malloc(sizeof(pid_t));
            if (pid_arg) {
                *pid_arg = pid;
                linx_thread_pool_add_task(g_process_cache->thread_pool, 
                                        update_process_task, pid_arg);
            }
        }
        
        closedir(proc_dir);
        
        /* 检查已缓存进程是否退出 */
        pthread_rwlock_wrlock(&g_process_cache->lock);
        process_info_t *info, *tmp;
        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            if (info->is_alive && !is_process_alive(info->pid)) {
                info->is_alive = 0;
                info->exit_time = time(NULL);
                info->state = PROCESS_STATE_EXITED;
            }
        }
        pthread_rwlock_unlock(&g_process_cache->lock);
        
        sleep(PROCESS_CACHE_UPDATE_INTERVAL);
    }
    
    return NULL;
}

/**
 * @brief 清理线程函数 - 定期清理过期缓存
 */
static void *cleaner_thread_func(void *arg)
{
    time_t now;
    process_info_t *info, *tmp;
    
    while (g_process_cache->running) {
        sleep(PROCESS_CACHE_EXPIRE_TIME / 2);
        
        now = time(NULL);
        
        pthread_rwlock_wrlock(&g_process_cache->lock);
        HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
            /* 清理已退出且超过保留时间的进程 */
            if (!info->is_alive && info->exit_time > 0 &&
                (now - info->exit_time) > PROCESS_CACHE_EXPIRE_TIME) {
                HASH_DEL(g_process_cache->hash_table, info);
                free_process_info(info);
            }
        }
        pthread_rwlock_unlock(&g_process_cache->lock);
    }
    
    return NULL;
}

/**
 * @brief 初始化进程缓存
 */
int linx_process_cache_init(void)
{
    if (g_process_cache) {
        return 0; /* 已初始化 */
    }
    
    g_process_cache = calloc(1, sizeof(process_cache_t));
    if (!g_process_cache) {
        return -1;
    }
    
    /* 初始化读写锁 */
    if (pthread_rwlock_init(&g_process_cache->lock, NULL) != 0) {
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }
    
    /* 创建线程池 */
    g_process_cache->thread_pool = linx_thread_pool_create(PROCESS_CACHE_THREAD_NUM);
    if (!g_process_cache->thread_pool) {
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }
    
    g_process_cache->running = 1;
    
    /* 创建监控线程 */
    if (pthread_create(&g_process_cache->monitor_thread, NULL, 
                      monitor_thread_func, NULL) != 0) {
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }
    
    /* 创建清理线程 */
    if (pthread_create(&g_process_cache->cleaner_thread, NULL,
                      cleaner_thread_func, NULL) != 0) {
        g_process_cache->running = 0;
        pthread_join(g_process_cache->monitor_thread, NULL);
        linx_thread_pool_destroy(g_process_cache->thread_pool, 0);
        pthread_rwlock_destroy(&g_process_cache->lock);
        free(g_process_cache);
        g_process_cache = NULL;
        return -1;
    }
    
    return 0;
}

/**
 * @brief 销毁进程缓存
 */
void linx_process_cache_destroy(void)
{
    if (!g_process_cache) {
        return;
    }
    
    /* 停止运行 */
    g_process_cache->running = 0;
    
    /* 等待线程退出 */
    pthread_join(g_process_cache->monitor_thread, NULL);
    pthread_join(g_process_cache->cleaner_thread, NULL);
    
    /* 销毁线程池 */
    linx_thread_pool_destroy(g_process_cache->thread_pool, 1);
    
    /* 清理哈希表 */
    pthread_rwlock_wrlock(&g_process_cache->lock);
    process_info_t *info, *tmp;
    HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
        HASH_DEL(g_process_cache->hash_table, info);
        free_process_info(info);
    }
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    /* 销毁锁 */
    pthread_rwlock_destroy(&g_process_cache->lock);
    
    /* 释放管理器 */
    free(g_process_cache);
    g_process_cache = NULL;
}

/**
 * @brief 获取进程信息
 */
process_info_t *linx_process_cache_get(pid_t pid)
{
    if (!g_process_cache) {
        return NULL;
    }
    
    process_info_t *info = NULL;
    
    pthread_rwlock_rdlock(&g_process_cache->lock);
    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    return info;
}

/**
 * @brief 手动更新指定进程缓存
 */
int linx_process_cache_update(pid_t pid)
{
    if (!g_process_cache) {
        return -1;
    }
    
    pid_t *pid_arg = malloc(sizeof(pid_t));
    if (!pid_arg) {
        return -1;
    }
    
    *pid_arg = pid;
    return linx_thread_pool_add_task(g_process_cache->thread_pool,
                                   update_process_task, pid_arg);
}

/**
 * @brief 删除指定进程缓存
 */
int linx_process_cache_delete(pid_t pid)
{
    if (!g_process_cache) {
        return -1;
    }
    
    process_info_t *info;
    
    pthread_rwlock_wrlock(&g_process_cache->lock);
    HASH_FIND_INT(g_process_cache->hash_table, &pid, info);
    if (info) {
        HASH_DEL(g_process_cache->hash_table, info);
        free_process_info(info);
    }
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    return 0;
}

/**
 * @brief 获取所有进程列表
 */
int linx_process_cache_get_all(process_info_t **list, int *count)
{
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
    
    *list = calloc(*count, sizeof(process_info_t));
    if (!*list) {
        pthread_rwlock_unlock(&g_process_cache->lock);
        return -1;
    }
    
    process_info_t *info;
    int i = 0;
    for (info = g_process_cache->hash_table; info != NULL; info = info->hh.next) {
        memcpy(&(*list)[i], info, sizeof(process_info_t));
        /* 清除哈希句柄，避免外部误用 */
        memset(&(*list)[i].hh, 0, sizeof(UT_hash_handle));
        i++;
    }
    
    pthread_rwlock_unlock(&g_process_cache->lock);
    return 0;
}

/**
 * @brief 清理过期缓存
 */
int linx_process_cache_cleanup(void)
{
    if (!g_process_cache) {
        return -1;
    }
    
    time_t now = time(NULL);
    process_info_t *info, *tmp;
    int cleaned = 0;
    
    pthread_rwlock_wrlock(&g_process_cache->lock);
    HASH_ITER(hh, g_process_cache->hash_table, info, tmp) {
        if (!info->is_alive && info->exit_time > 0 &&
            (now - info->exit_time) > PROCESS_CACHE_EXPIRE_TIME) {
            HASH_DEL(g_process_cache->hash_table, info);
            free_process_info(info);
            cleaned++;
        }
    }
    pthread_rwlock_unlock(&g_process_cache->lock);
    
    return cleaned;
}

/**
 * @brief 获取缓存统计信息
 */
void linx_process_cache_stats(int *total, int *alive, int *expired)
{
    if (!g_process_cache) {
        if (total) *total = 0;
        if (alive) *alive = 0;
        if (expired) *expired = 0;
        return;
    }
    
    int t = 0, a = 0, e = 0;
    process_info_t *info;
    
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
}