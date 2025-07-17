#include <stdlib.h>
#include <unistd.h>

#include "linx_thread_pool.h"

/**
 * @brief 线程池工作线程的主函数
 * 
 * 每个工作线程都会执行此函数，主要职责是从任务队列中获取任务并执行。
 * 线程可以处于运行、暂停或终止状态，根据状态决定是否处理任务或退出。
 * 
 * @param arg 指向线程池结构体的指针，包含线程池的所有信息
 * @return void* 始终返回NULL，无实际意义
 */
static void *linx_thread_worker(void *arg)
{
    linx_thread_pool_t *pool = (linx_thread_pool_t *)arg;
    linx_thread_info_t *thread = NULL;
    linx_task_t *task = NULL;

    /* 锁定线程池，查找当前线程对应的线程信息结构体 */
    pthread_mutex_lock(&pool->lock);

    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_equal(pthread_self(), pool->threads[i].thread_id)) {
            thread = &pool->threads[i];
            break;
        }
    }

    pthread_mutex_unlock(&pool->lock);

    /* 如果未找到对应的线程信息，直接返回 */
    if (!thread) {
        return NULL;
    }

    /* 设置当前线程状态为运行中 */
    pthread_mutex_lock(&thread->state_mutex);
    thread->state = LINX_THREAD_STATE_RUNNING;
    pthread_mutex_unlock(&thread->state_mutex);

    /* 主循环：不断从任务队列中获取并执行任务 */
    while (1) {
        pthread_mutex_lock(&thread->state_mutex);

        /* 如果线程被暂停，等待恢复信号 */
        while (thread->state == LINX_THREAD_STATE_PAUSED) {
            pthread_cond_wait(&thread->pause_cond, &thread->state_mutex);
        }
        
        /* 如果收到终止信号，退出循环 */
        if (thread->state == LINX_THREAD_STATE_TERMINATING) {
            pthread_mutex_unlock(&thread->state_mutex);
            break;
        }

        pthread_mutex_unlock(&thread->state_mutex);

        /* 锁定线程池，检查任务队列 */
        pthread_mutex_lock(&pool->lock);

        /* 如果任务队列为空且线程池未关闭，等待新任务通知 */
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        /* 检查线程池关闭条件：立即关闭或优雅关闭且任务队列为空 */
        if (pool->shutdown == 2 || (pool->shutdown == 1 && pool->queue_size == 0)) { 
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        /* 从任务队列头部获取任务 */
        task = pool->task_queue_head;
        if (task == NULL) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }

        pthread_mutex_unlock(&pool->lock);

        /* 执行任务并释放任务结构体 */
        (*(task->func))(task->arg, &task->should_stop);

        pthread_mutex_lock(&pool->lock);

        /* 调整任务队列指针和大小 */
        pool->task_queue_head = task->next;
        if (pool->task_queue_head == NULL) {
            pool->task_queue_tail = NULL;
        }
        
        pool->queue_size--;

        pthread_mutex_unlock(&pool->lock);

        free(task);
    }
    
    /* 设置线程状态为已终止 */
    pthread_mutex_lock(&thread->state_mutex);

    thread->state = LINX_THREAD_STATE_TERMINATED;

    pthread_mutex_unlock(&thread->state_mutex);

    /* 减少线程池中的活动线程计数 */
    pthread_mutex_lock(&pool->lock);

    pool->active_threads--;

    pthread_mutex_unlock(&pool->lock);

    return NULL;
}

/**
 * @brief 创建并初始化一个线程池
 * 
 * 该函数创建一个包含指定数量线程的线程池。如果输入参数num_threads小于等于0，
 * 则自动使用系统CPU核数作为线程数量。线程池包含线程信息数组、互斥锁和条件变量等资源。
 * 
 * @param num_threads 线程池中线程的数量。若<=0则使用系统CPU核数。
 * @return linx_thread_pool_t* 成功返回创建的线程池指针，失败返回NULL。
 */
linx_thread_pool_t *linx_thread_pool_create(int num_threads)
{
    linx_thread_pool_t *pool;
    linx_thread_info_t *thread;

    /* 处理线程数量参数：若无效则使用系统CPU核数，至少保证1个线程 */
    if (num_threads <= 0) {
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_threads <= 0) {
            num_threads = 1;
        }
    }

    /* 分配线程池主结构内存 */
    pool = (linx_thread_pool_t *)malloc(sizeof(linx_thread_pool_t));
    if (pool == NULL) {
        return NULL;
    }

    pool->thread_count = num_threads;
    pool->active_threads = num_threads;
    pool->queue_size = 0;
    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->shutdown = 0;

    /* 分配线程信息数组内存 */
    pool->threads = (linx_thread_info_t *)malloc(sizeof(linx_thread_info_t) * num_threads);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    /* 初始化线程池全局锁 */
    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    /* 初始化线程池全局通知条件变量 */
    if (pthread_cond_init(&(pool->notify), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        free(pool->threads);
        free(pool);
        return NULL;
    }

    /* 初始化每个线程的私有资源（状态锁和暂停条件变量） */
    for (int i = 0; i < num_threads; i++) {
        thread = &(pool->threads[i]);
        thread->index = i;
        thread->state = LINX_THREAD_STATE_RUNNING;

        /* 初始化线程状态锁 */
        if (pthread_mutex_init(&(thread->state_mutex), NULL) != 0) {
            /* 失败时清理已创建的资源 */
            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&(pool->threads[j].state_mutex));
                pthread_cond_destroy(&(pool->threads[j].pause_cond));
            }

            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));

            free(pool->threads);
            free(pool);

            return NULL;
        }

        /* 初始化线程暂停条件变量 */
        if (pthread_cond_init(&(thread->pause_cond), NULL) != 0) {
            /* 失败时清理已创建的资源 */
            pthread_mutex_destroy(&(thread->state_mutex));

            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&(pool->threads[j].state_mutex));
                pthread_cond_destroy(&(pool->threads[j].pause_cond));
            }

            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));

            free(pool->threads);
            free(pool);

            return NULL;
        }
    }

    /* 创建所有工作线程 */
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&(pool->threads[i].thread_id), NULL, linx_thread_worker, (void *)pool) != 0) {
            /* 失败时取消已创建的线程并清理资源 */
            for (int j = 0; j < i; j++) {
                pthread_cancel(pool->threads[j].thread_id);
                pthread_join(pool->threads[j].thread_id, NULL);
            }

            for (int j = 0; j < num_threads; j++) {
                pthread_mutex_destroy(&(pool->threads[j].state_mutex));
                pthread_cond_destroy(&(pool->threads[j].pause_cond));
            }

            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));

            free(pool->threads);
            free(pool);

            return NULL;
        }
    }

    return pool;
}

/**
 * 销毁线程池
 * 
 * @param pool      指向要销毁的线程池的指针
 * @param graceful  优雅关闭标志：
 *                  非0表示优雅关闭(等待当前任务完成)
 *                  0表示强制立即关闭
 * 
 * @return 成功返回0，失败返回-1
 *         失败情况包括：参数无效、互斥锁操作失败、线程池已关闭等
 */
int linx_thread_pool_destroy(linx_thread_pool_t *pool, int graceful)
{
    linx_thread_info_t *thread = NULL;
    linx_task_t *task = NULL;

    /* 参数有效性检查 */
    if (pool == NULL) {
        return -1;
    }

    /* 获取线程池锁 */
    if (pthread_mutex_lock(&(pool->lock))) {
        return -1;
    }

    /* 检查线程池是否已关闭 */
    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    /* 设置关闭模式：1=优雅关闭，2=强制关闭 */
    pool->shutdown = (graceful) ? 1 : 2;

    /**
     * 将所有任务都标记为停止，避免死循环任务一直执行
     * 死循环任务中可判断 shutdown
     * 1 : 将未执行完的任务执行完再退出循环
     * 2 : 立马退出循环
    */
    task = pool->task_queue_head;
    while (task != NULL) {
        task->should_stop = pool->shutdown;
        task = task->next;
    }

    /* 广播通知所有线程 */
    if (pthread_cond_broadcast(&(pool->notify))) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    pthread_mutex_unlock(&(pool->lock));

    /* 通知所有线程进入终止状态 */
    for (int i = 0; i < pool->thread_count; i++) {
        thread = &(pool->threads[i]);

        pthread_mutex_lock(&(thread->state_mutex));

        if (thread->state != LINX_THREAD_STATE_TERMINATED) {
            /* 如果线程处于暂停状态，唤醒它 */
            if (thread->state == LINX_THREAD_STATE_PAUSED) {
                pthread_cond_signal(&(thread->pause_cond));
            }

            thread->state = LINX_THREAD_STATE_TERMINATING;
        }

        pthread_mutex_unlock(&(thread->state_mutex));
    }

    /* 等待所有线程结束 */
    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i].thread_id, NULL)) {
            /* 忽略线程join失败的情况 */
        }
    }

    /* 销毁线程池的同步对象 */
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));

    /* 销毁每个线程的同步对象 */
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_mutex_destroy(&(pool->threads[i].state_mutex));
        pthread_cond_destroy(&(pool->threads[i].pause_cond));
    }

    /* 清理任务队列 */
    while (pool->task_queue_head != NULL) {
        task = pool->task_queue_head;
        pool->task_queue_head = task->next;
        free(task);
    }
    
    /* 释放线程池资源 */
    free(pool->threads);
    free(pool);

    return 0;
}

/**
 * @brief 向线程池中添加任务
 * 
 * 该函数将一个任务添加到线程池的任务队列中，并通知等待的线程有新任务可用。
 * 
 * @param pool 指向线程池结构的指针，不能为NULL
 * @param func 任务函数指针，该函数将被线程池中的线程执行，不能为NULL
 * @param arg 传递给任务函数的参数
 * @return int 成功返回0，失败返回-1
 *             -1表示参数无效、线程池已关闭、内存分配失败或锁操作失败
 */
int linx_thread_pool_add_task(linx_thread_pool_t *pool, void *(*func)(void *, int *), void *arg)
{
    linx_task_t *task = NULL;

    /* 参数有效性检查 */
    if (pool == NULL || func == NULL) {
        return -1;
    }

    /* 获取线程池锁 */
    if (pthread_mutex_lock(&(pool->lock))) {
        return -1;
    }

    /* 检查线程池是否已关闭 */
    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    /* 分配任务结构体内存 */
    task = (linx_task_t *)malloc(sizeof(linx_task_t));
    if (task == NULL) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    /* 初始化任务结构体 */
    task->func = func;
    task->arg = arg;
    task->should_stop = 0;
    task->next = NULL;

    /* 将任务添加到队列尾部 */
    if (pool->task_queue_head == NULL) {
        pool->task_queue_head = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    }

    pool->queue_size++;

    /* 通知等待的线程有新任务 */
    if (pthread_cond_signal(&(pool->notify))) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

/**
 * @brief 暂停线程池中指定线程的运行
 * 
 * 该函数将线程池中指定索引的线程状态从RUNNING改为PAUSED。如果线程当前不是RUNNING状态，
 * 则不做任何操作。函数会确保线程状态修改的线程安全性。
 * 
 * @param pool 指向线程池结构的指针，不能为NULL
 * @param thread_index 要暂停的线程索引，必须有效(0 <= index < thread_count)
 * @return int 执行结果：0表示成功，-1表示参数无效
 */
int linx_thread_pool_pause_thread(linx_thread_pool_t *pool, int thread_index)
{
    linx_thread_info_t *thread = NULL;

    /* 参数有效性检查：确保线程池指针有效且线程索引在合法范围内 */
    if (pool == NULL || thread_index < 0 || thread_index >= pool->thread_count) {
        return -1;
    }

    thread = &pool->threads[thread_index];

    /* 线程安全地修改线程状态 */
    pthread_mutex_lock(&(thread->state_mutex));

    if (thread->state == LINX_THREAD_STATE_RUNNING) {
        thread->state = LINX_THREAD_STATE_PAUSED;
    }

    pthread_mutex_unlock(&(thread->state_mutex));

    return 0;
}

/**
 * @brief 恢复线程池中指定暂停状态的线程
 *
 * 该函数用于将线程池中处于暂停状态的指定线程恢复为运行状态。
 * 如果线程当前不是暂停状态，则不做任何操作。
 *
 * @param pool         线程池指针，不能为NULL
 * @param thread_index 要恢复的线程索引，必须有效(0 <= index < thread_count)
 *
 * @return 成功返回0，失败返回-1(参数无效时)
 */
int linx_thread_pool_resume_thread(linx_thread_pool_t *pool, int thread_index)
{
    linx_thread_info_t *thread = NULL;

    /* 参数有效性检查 */
    if (pool == NULL || thread_index < 0 || thread_index >= pool->thread_count) {
        return -1;
    }

    /* 获取指定线程的信息 */
    thread = &pool->threads[thread_index];

    /* 修改线程状态需要先获取锁 */
    pthread_mutex_lock(&(thread->state_mutex));

    if (thread->state == LINX_THREAD_STATE_PAUSED) {
        /* 将暂停状态改为运行状态，并发送信号唤醒可能正在等待的线程 */
        thread->state = LINX_THREAD_STATE_RUNNING;
        pthread_cond_signal(&(thread->pause_cond));
    }

    pthread_mutex_unlock(&(thread->state_mutex));

    return 0;
}

/**
 * @brief 终止线程池中的指定线程
 *
 * 该函数用于终止线程池中指定索引的线程。函数会先检查参数有效性，然后根据线程当前状态
 * 进行不同的终止处理：
 * 1. 对于运行中的线程，直接设置终止标志
 * 2. 对于暂停中的线程，先唤醒线程再设置终止标志
 *
 * @param pool        线程池指针，不能为NULL
 * @param thread_index 要终止的线程索引，必须有效(0 <= index < thread_count)
 * @return int 操作结果：
 *             - 0 表示成功
 *             - -1 表示参数无效(pool为NULL或thread_index越界)
 */
int linx_thread_pool_terminate_thread(linx_thread_pool_t *pool, int thread_index)
{
    linx_thread_info_t *thread = NULL;

    /* 参数校验：线程池指针和线程索引范围 */
    if (pool == NULL || thread_index < 0 || thread_index >= pool->thread_count) {
        return -1;
    }

    /* 获取指定线程的控制结构 */
    thread = &(pool->threads[thread_index]);

    /* 锁定线程状态互斥锁以保证线程安全 */
    pthread_mutex_lock(&(thread->state_mutex));

    /* 仅处理运行中或暂停状态的线程 */
    if (thread->state == LINX_THREAD_STATE_RUNNING ||
        thread->state == LINX_THREAD_STATE_PAUSED) 
    {
        /* 如果是暂停状态线程，先发送唤醒信号 */
        if (thread->state == LINX_THREAD_STATE_PAUSED) {
            pthread_cond_signal(&(thread->pause_cond));
        }

        /* 设置线程终止标志 */
        thread->state = LINX_THREAD_STATE_TERMINATING;
    }

    /* 解锁线程状态互斥锁 */
    pthread_mutex_unlock(&(thread->state_mutex));

    return 0;
}

/**
 * @brief 获取线程池中指定线程的当前状态
 * 
 * 该函数用于查询线程池中指定索引位置线程的运行状态。函数首先进行参数校验，
 * 确保线程池指针有效且线程索引在合法范围内。通过线程状态互斥锁保护状态读取操作，
 * 确保线程安全地获取状态值。
 * 
 * @param pool 指向线程池结构的指针，不能为NULL
 * @param thread_index 要查询的线程索引，必须 >=0且 < pool->thread_count
 * @return linx_thread_state_t 返回线程状态枚举值：
 *             - 成功时返回线程当前状态（运行中/暂停中/终止中/已终止）
 *             - 如果参数无效则返回-1表示错误
 */
linx_thread_state_t linx_thread_pool_get_thread_state(linx_thread_pool_t *pool, int thread_index)
{
    linx_thread_info_t *thread = NULL;
    linx_thread_state_t state;

    /* 参数有效性检查：线程池指针和线程索引范围 */
    if (pool == NULL || thread_index < 0 || thread_index >= pool->thread_count) {
        return -1;
    }

    /* 获取指定索引的线程信息结构体 */
    thread = &(pool->threads[thread_index]);

    /* 加锁获取线程状态（确保线程安全） */
    pthread_mutex_lock(&(thread->state_mutex));

    state = thread->state;

    pthread_mutex_unlock(&(thread->state_mutex));

    return state;
}

/**
 * @brief 获取线程池当前的任务队列大小
 * 
 * 该函数用于查询线程池中当前待处理任务的数量。函数会先检查线程池指针的有效性，
 * 然后通过互斥锁保护共享数据，获取队列大小后释放锁。
 * 
 * @param pool 指向线程池结构的指针，不能为NULL
 * @return int 返回队列中任务的数量：
 *             - 成功时返回非负整数表示队列大小
 *             - 如果pool为NULL则返回-1表示错误
 */
int linx_thread_pool_get_queue_size(linx_thread_pool_t *pool)
{
    int queue_size;

    /* 检查输入参数有效性 */
    if (pool == NULL) {
        return -1;
    }

    /* 加锁保护共享数据 */
    pthread_mutex_lock(&(pool->lock));

    queue_size = pool->queue_size;

    pthread_mutex_unlock(&(pool->lock));

    return queue_size;
}

/**
 * @brief 获取线程池中当前活跃的线程数量
 * 
 * 该函数用于查询线程池中当前活跃的线程数量。在获取数量前会先锁定线程池的互斥锁，
 * 以确保线程安全。如果传入的线程池指针为空，则返回错误码。
 * 
 * @param pool 指向线程池结构体的指针，不能为NULL
 * @return int 返回当前活跃线程数量；如果pool为NULL则返回-1表示错误
 */
int linx_thread_pool_get_active_threads(linx_thread_pool_t *pool)
{
    int count;

    /* 检查线程池指针有效性 */
    if (pool == NULL) {
        return -1;
    }

    /* 锁定线程池互斥锁以确保线程安全 */
    pthread_mutex_lock(&(pool->lock));

    /* 获取当前线程数量 */
    count = pool->thread_count;

    /* 解锁线程池互斥锁 */
    pthread_mutex_unlock(&(pool->lock));

    return count;
}
