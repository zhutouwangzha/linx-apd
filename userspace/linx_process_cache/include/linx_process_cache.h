#ifndef __LINX_PROCESS_CACHE_H__
#define __LINX_PROCESS_CACHE_H__ 

#include <pthread.h>

#include "linx_process_cache_info.h"
#include "linx_thread_pool.h"

typedef struct {
    pthread_rwlock_t lock;
    linx_process_info_t *hash_table;
    linx_thread_pool_t *thread_pool;
    int running;
    int inotify_fd;                    /* inotify文件描述符 */
    int proc_watch_fd;                 /* /proc目录的监控描述符 */
} linx_process_cache_t;

int linx_process_cache_init(void);

void linx_process_cache_deinit(void);

linx_process_info_t *linx_process_cache_get(pid_t pid);

int linx_process_cache_get_all(linx_process_info_t **list, int *count);

int linx_process_cache_update_async(pid_t pid);

int linx_process_cache_update_sync(pid_t pid);

int linx_process_cache_delete(pid_t pid);

int linx_process_cache_cleanup(void);

void linx_process_cache_stats(int *total, int *alive, int *expired);

int linx_process_cache_get_monitor_status(char *status_buf, size_t buf_size);

#endif /* __LINX_PROCESS_CACHE_H__ */
