#ifndef __LINX_PROCESS_CACHE_H__
#define __LINX_PROCESS_CACHE_H__ 

#include <pthread.h>

#include "linx_process_cache_info.h"
#include "linx_thread_pool.h"

typedef struct {
    linx_process_info_t *hash_table;
    pthread_rwlock_t lock;

    linx_thread_pool_t *thread_pool;
    int running;
    pthread_t monitor_thread;
    pthread_t cleaner_thread;
} linx_process_cache_t;

int linx_process_cache_init(void);

void linx_process_cache_deinit(void);

linx_process_info_t *linx_process_cache_get(pid_t pid);

int linx_process_cache_get_all(linx_process_info_t **list, int *count);

int linx_process_cache_update(pid_t pid);

int linx_process_cache_delete(pid_t pid);

int linx_process_cache_cleanup(void);

void linx_process_cache_stats(int *total, int *alive, int *expired);

/* 新增事件驱动接口 */
int linx_process_cache_on_fork(pid_t new_pid, pid_t parent_pid);

int linx_process_cache_on_exec(pid_t pid, const char *filename, const char *argv, const char *envp);

int linx_process_cache_on_exit(pid_t pid, int exit_code);

int linx_process_cache_preload(pid_t pid);

#endif /* __LINX_PROCESS_CACHE_H__ */
