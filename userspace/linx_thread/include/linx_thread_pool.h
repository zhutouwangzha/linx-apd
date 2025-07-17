#ifndef __LINX_THREAD_POOL_H__
#define __LINX_THREAD_POOL_H__ 

#include "linx_thread_info.h"

typedef struct linx_task_s {
    void *(*func)(void *, int *);
    void *arg;
    int should_stop;
    struct linx_task_s *next;
} linx_task_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    linx_thread_info_t *threads;
    linx_task_t *task_queue_head;
    linx_task_t *task_queue_tail;
    int thread_count;
    int active_threads;
    int queue_size;
    int shutdown;
} linx_thread_pool_t;

linx_thread_pool_t *linx_thread_pool_create(int num_threads);

int linx_thread_pool_destroy(linx_thread_pool_t *pool, int graceful);

int linx_thread_pool_add_task(linx_thread_pool_t *pool, void *(*func)(void *, int *), void *arg);

int linx_thread_pool_pause_thread(linx_thread_pool_t *pool, int thread_index);

int linx_thread_pool_resume_thread(linx_thread_pool_t *pool, int thread_index);

int linx_thread_pool_terminate_thread(linx_thread_pool_t *pool, int thread_index);

linx_thread_state_t linx_thread_pool_get_thread_state(linx_thread_pool_t *pool, int thread_index);

int linx_thread_pool_get_queue_size(linx_thread_pool_t *pool);

int linx_thread_pool_get_active_threads(linx_thread_pool_t *pool);

#endif /* __LINX_THREAD_POOL_H__ */
