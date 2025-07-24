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

#endif /* __LINX_PROCESS_CACHE_H__ */
