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
#include <sys/inotify.h>
#include <sys/epoll.h>

#include "linx_process_cache.h"
#include "linx_process_cache_ebpf.h"

/* 扩展配置 */
#define FAST_SCAN_INTERVAL_MS       100     /* 快速扫描间隔(毫秒) */
#define SHORT_PROCESS_THRESHOLD_MS  200     /* 短进程阈值(毫秒) */
#define PROC_SCAN_BATCH_SIZE        256     /* 批量扫描大小 */

/* 短进程检测相关 */
typedef struct short_process_info {
    pid_t pid;
    time_t detection_time;
    char comm[32];
    struct short_process_info *next;
} short_process_info_t;

/* 全局变量 */
static short_process_info_t *g_short_process_list = NULL;
static pthread_mutex_t g_short_process_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_fast_scan_enabled = 0;
static pthread_t g_fast_scan_thread;
static pthread_t g_inotify_thread;

/* inotify相关 */
static int g_inotify_fd = -1;
static int g_proc_wd = -1;

/**
 * @brief 添加短进程记录
 */
static void add_short_process_record(pid_t pid, const char *comm)
{
    short_process_info_t *info = malloc(sizeof(short_process_info_t));
    if (!info) {
        return;
    }
    
    info->pid = pid;
    info->detection_time = time(NULL);
    strncpy(info->comm, comm ? comm : "unknown", sizeof(info->comm) - 1);
    info->comm[sizeof(info->comm) - 1] = '\0';
    
    pthread_mutex_lock(&g_short_process_lock);
    info->next = g_short_process_list;
    g_short_process_list = info;
    pthread_mutex_unlock(&g_short_process_lock);
}

/**
 * @brief 检查PID是否曾经存在过（通过检查/proc/sys/kernel/pid_max等）
 */
static int pid_has_existed(pid_t pid)
{
    char path[256];
    struct stat st;
    
    /* 检查/proc/[pid]目录是否曾经存在过的痕迹 */
    snprintf(path, sizeof(path), "/proc/%d", pid);
    
    /* 如果目录存在，说明进程还在运行 */
    if (stat(path, &st) == 0) {
        return 1;
    }
    
    /* 检查内核日志或其他痕迹（这里可以扩展更多检测方法） */
    /* 例如检查 /var/log/kern.log 或使用 dmesg */
    
    return 0;
}

/**
 * @brief 快速扫描线程 - 高频率检测短进程
 */
static void *fast_scan_thread_func(void *arg)
{
    (void)arg;
    
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid;
    static pid_t last_max_pid = 0;
    pid_t current_max_pid = 0;
    
    while (g_fast_scan_enabled) {
        proc_dir = opendir("/proc");
        if (!proc_dir) {
            usleep(FAST_SCAN_INTERVAL_MS * 1000);
            continue;
        }
        
        /* 扫描/proc目录寻找新进程 */
        while ((entry = readdir(proc_dir)) != NULL) {
            if (!isdigit(entry->d_name[0])) {
                continue;
            }
            
            pid = atoi(entry->d_name);
            if (pid <= 0) {
                continue;
            }
            
            /* 跟踪最大PID */
            if (pid > current_max_pid) {
                current_max_pid = pid;
            }
            
            /* 检查是否为新进程 */
            if (pid > last_max_pid) {
                char stat_path[256];
                char comm[32] = {0};
                FILE *fp;
                
                /* 尝试读取进程信息 */
                snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
                fp = fopen(stat_path, "r");
                if (fp) {
                    /* 读取进程名 */
                    fscanf(fp, "%*d (%31[^)])", comm);
                    fclose(fp);
                    
                    /* 立即更新缓存 */
                    linx_process_cache_update(pid);
                }
            }
        }
        
        closedir(proc_dir);
        last_max_pid = current_max_pid;
        
        /* 短暂休眠 */
        usleep(FAST_SCAN_INTERVAL_MS * 1000);
    }
    
    return NULL;
}

/**
 * @brief inotify监控线程 - 监控/proc目录变化
 */
static void *inotify_thread_func(void *arg)
{
    (void)arg;
    
    char buffer[4096];
    int length;
    int epoll_fd;
    struct epoll_event event, events[10];
    
    /* 创建epoll */
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        return NULL;
    }
    
    /* 添加inotify fd到epoll */
    event.events = EPOLLIN;
    event.data.fd = g_inotify_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, g_inotify_fd, &event) == -1) {
        close(epoll_fd);
        return NULL;
    }
    
    while (g_fast_scan_enabled) {
        int nfds = epoll_wait(epoll_fd, events, 10, 1000); /* 1秒超时 */
        
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == g_inotify_fd) {
                length = read(g_inotify_fd, buffer, sizeof(buffer));
                if (length < 0) {
                    continue;
                }
                
                /* 处理inotify事件 */
                int offset = 0;
                while (offset < length) {
                    struct inotify_event *event = (struct inotify_event *)&buffer[offset];
                    
                    if (event->len > 0 && isdigit(event->name[0])) {
                        pid_t pid = atoi(event->name);
                        
                        if (event->mask & IN_CREATE) {
                            /* 新进程创建 */
                            linx_process_cache_update(pid);
                        } else if (event->mask & IN_DELETE) {
                            /* 进程目录删除 */
                            process_info_t *info = linx_process_cache_get(pid);
                            if (info) {
                                info->is_alive = 0;
                                info->exit_time = time(NULL);
                                info->state = PROCESS_STATE_EXITED;
                            }
                        }
                    }
                    
                    offset += sizeof(struct inotify_event) + event->len;
                }
            }
        }
    }
    
    close(epoll_fd);
    return NULL;
}

/**
 * @brief 启用快速扫描模式
 */
int linx_process_cache_enable_fast_scan(void)
{
    if (g_fast_scan_enabled) {
        return 0; /* 已启用 */
    }
    
    /* 初始化inotify */
    g_inotify_fd = inotify_init1(IN_NONBLOCK);
    if (g_inotify_fd == -1) {
        fprintf(stderr, "Failed to initialize inotify: %s\n", strerror(errno));
        return -1;
    }
    
    /* 监控/proc目录 */
    g_proc_wd = inotify_add_watch(g_inotify_fd, "/proc", IN_CREATE | IN_DELETE);
    if (g_proc_wd == -1) {
        fprintf(stderr, "Failed to add watch on /proc: %s\n", strerror(errno));
        close(g_inotify_fd);
        return -1;
    }
    
    g_fast_scan_enabled = 1;
    
    /* 启动快速扫描线程 */
    if (pthread_create(&g_fast_scan_thread, NULL, fast_scan_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create fast scan thread\n");
        g_fast_scan_enabled = 0;
        inotify_rm_watch(g_inotify_fd, g_proc_wd);
        close(g_inotify_fd);
        return -1;
    }
    
    /* 启动inotify监控线程 */
    if (pthread_create(&g_inotify_thread, NULL, inotify_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create inotify thread\n");
        g_fast_scan_enabled = 0;
        pthread_join(g_fast_scan_thread, NULL);
        inotify_rm_watch(g_inotify_fd, g_proc_wd);
        close(g_inotify_fd);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 禁用快速扫描模式
 */
void linx_process_cache_disable_fast_scan(void)
{
    if (!g_fast_scan_enabled) {
        return;
    }
    
    g_fast_scan_enabled = 0;
    
    /* 等待线程结束 */
    pthread_join(g_fast_scan_thread, NULL);
    pthread_join(g_inotify_thread, NULL);
    
    /* 清理inotify */
    if (g_proc_wd != -1) {
        inotify_rm_watch(g_inotify_fd, g_proc_wd);
    }
    if (g_inotify_fd != -1) {
        close(g_inotify_fd);
    }
    
    /* 清理短进程列表 */
    pthread_mutex_lock(&g_short_process_lock);
    short_process_info_t *current = g_short_process_list;
    while (current) {
        short_process_info_t *next = current->next;
        free(current);
        current = next;
    }
    g_short_process_list = NULL;
    pthread_mutex_unlock(&g_short_process_lock);
}

/**
 * @brief 获取短进程列表
 */
int linx_process_cache_get_short_processes(short_process_info_t **list, int *count)
{
    if (!list || !count) {
        return -1;
    }
    
    *list = NULL;
    *count = 0;
    
    pthread_mutex_lock(&g_short_process_lock);
    
    short_process_info_t *current = g_short_process_list;
    while (current) {
        (*count)++;
        current = current->next;
    }
    
    if (*count > 0) {
        *list = malloc(sizeof(short_process_info_t) * (*count));
        if (*list) {
            int i = 0;
            current = g_short_process_list;
            while (current && i < *count) {
                (*list)[i] = *current;
                (*list)[i].next = NULL; /* 清除链接 */
                current = current->next;
                i++;
            }
        } else {
            *count = 0;
        }
    }
    
    pthread_mutex_unlock(&g_short_process_lock);
    
    return 0;
}

/**
 * @brief eBPF事件处理器
 */
static int handle_process_event(const struct process_event *event)
{
    switch (event->event_type) {
        case PROCESS_EVENT_FORK:
        case PROCESS_EVENT_EXEC:
            /* 进程创建，立即缓存 */
            linx_process_cache_update(event->pid);
            break;
            
        case PROCESS_EVENT_EXIT:
            /* 进程退出，检查是否为短进程 */
            {
                unsigned long long runtime_ns = event->exit_time - event->start_time;
                if (runtime_ns < SHORT_PROCESS_THRESHOLD_MS * 1000000) {
                    /* 这是一个短进程，记录下来 */
                    add_short_process_record(event->pid, event->comm);
                }
                
                /* 更新缓存中的退出信息 */
                process_info_t *info = linx_process_cache_get(event->pid);
                if (info) {
                    info->is_alive = 0;
                    info->exit_time = event->exit_time / 1000000000;
                    info->state = PROCESS_STATE_EXITED;
                }
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

/**
 * @brief 初始化增强型进程缓存
 */
int linx_process_cache_enhanced_init(void)
{
    int ret;
    
    /* 初始化基础进程缓存 */
    ret = linx_process_cache_init();
    if (ret != 0) {
        return ret;
    }
    
    /* 初始化eBPF监控 */
    ret = linx_process_ebpf_init();
    if (ret == 0) {
        /* 注册事件处理器 */
        linx_process_ebpf_register_handler(handle_process_event);
        
        /* 启动eBPF消费者 */
        linx_process_ebpf_start_consumer();
        
        printf("eBPF process monitoring enabled\n");
    } else {
        printf("eBPF not available, falling back to enhanced polling\n");
        
        /* 启用快速扫描作为备选方案 */
        ret = linx_process_cache_enable_fast_scan();
        if (ret != 0) {
            printf("Fast scan mode also failed, using standard polling\n");
        }
    }
    
    return 0;
}

/**
 * @brief 销毁增强型进程缓存
 */
void linx_process_cache_enhanced_destroy(void)
{
    /* 停止eBPF监控 */
    linx_process_ebpf_destroy();
    
    /* 停止快速扫描 */
    linx_process_cache_disable_fast_scan();
    
    /* 销毁基础进程缓存 */
    linx_process_cache_destroy();
}