#ifndef __LINX_PROCESS_CACHE_H__
#define __LINX_PROCESS_CACHE_H__ 

#include <pthread.h>
#include <stdatomic.h>

#include "linx_process_cache_info.h"
#include "linx_thread_pool.h"

typedef struct {
    linx_process_info_t *hash_table;
    pthread_rwlock_t lock;

    linx_thread_pool_t *thread_pool;
    linx_thread_pool_t *fast_scan_pool;
    
    atomic_int running;
    atomic_int high_freq_mode;
    time_t high_freq_start_time;
    
    pthread_t monitor_thread;
    pthread_t cleaner_thread;
    pthread_t *fast_scan_threads;
    
    /* 统计信息 */
    atomic_long total_scanned;
    atomic_long short_lived_cached;
    atomic_long scan_cycles;
} linx_process_cache_t;

/* 快速扫描任务结构 */
typedef struct {
    int start_pid;
    int end_pid;
    int thread_id;
} fast_scan_task_t;

int linx_process_cache_init(void);

void linx_process_cache_deinit(void);

linx_process_info_t *linx_process_cache_get(pid_t pid);

int linx_process_cache_get_all(linx_process_info_t **list, int *count);

int linx_process_cache_update(pid_t pid);

int linx_process_cache_delete(pid_t pid);

int linx_process_cache_cleanup(void);

void linx_process_cache_stats(int *total, int *alive, int *expired);

/* 新增高性能接口 */
int linx_process_cache_force_update(pid_t pid);

int linx_process_cache_batch_update(pid_t *pids, int count);

void linx_process_cache_get_detailed_stats(int *total, int *alive, int *expired, 
                                         long *scanned, long *short_lived, long *cycles);

int linx_process_cache_set_high_freq_mode(int enabled);

#endif /* __LINX_PROCESS_CACHE_H__ */
