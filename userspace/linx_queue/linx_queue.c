#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "linx_queue.h"

/**
 * @brief 队列内部结构体
 */
struct linx_queue {
    /* 数据存储 */
    void **data;                 // 数据指针数组
    uint32_t head;               // 队头指针
    uint32_t tail;               // 队尾指针
    uint32_t size;               // 当前元素数量
    uint32_t capacity;           // 当前容量
    
    /* 配置信息 */
    linx_queue_config_t config;
    
    /* 统计信息 */
    linx_queue_stats_t stats;
    
    /* 线程同步 */
    pthread_mutex_t mutex;       // 互斥锁
    pthread_cond_t not_empty;    // 非空条件变量
    pthread_cond_t not_full;     // 非满条件变量
    bool interrupted;            // 中断标志
};

/**
 * @brief 获取默认配置
 */
void linx_queue_get_default_config(linx_queue_config_t *config)
{
    if (!config) return;
    
    config->initial_capacity = 16;
    config->max_capacity = 0;        // 无限制
    config->auto_resize = true;
    config->resize_factor = 2;
    config->thread_safe = true;
    config->use_condition = true;
}

/**
 * @brief 计算下一个队列位置
 */
static inline uint32_t linx_queue_next_index(uint32_t index, uint32_t capacity)
{
    return (index + 1) % capacity;
}

/**
 * @brief 扩容队列
 */
static linx_queue_result_t linx_queue_resize(linx_queue_t *queue, uint32_t new_capacity)
{
    void **new_data;
    uint32_t i, old_index;
    
    if (!queue->config.auto_resize) {
        return LINX_QUEUE_FULL;
    }
    
    if (queue->config.max_capacity > 0 && new_capacity > queue->config.max_capacity) {
        return LINX_QUEUE_FULL;
    }
    
    new_data = malloc(sizeof(void *) * new_capacity);
    if (!new_data) {
        return LINX_QUEUE_ERROR;
    }
    
    /* 重新排列元素到新数组 */
    for (i = 0; i < queue->size; i++) {
        old_index = (queue->head + i) % queue->capacity;
        new_data[i] = queue->data[old_index];
    }
    
    free(queue->data);
    queue->data = new_data;
    queue->head = 0;
    queue->tail = queue->size;
    queue->capacity = new_capacity;
    queue->stats.total_resizes++;
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 创建队列
 */
linx_queue_t *linx_queue_create(const linx_queue_config_t *config)
{
    linx_queue_t *queue;
    linx_queue_config_t default_config;
    
    queue = calloc(1, sizeof(linx_queue_t));
    if (!queue) {
        return NULL;
    }
    
    /* 使用提供的配置或默认配置 */
    if (config) {
        queue->config = *config;
    } else {
        linx_queue_get_default_config(&default_config);
        queue->config = default_config;
    }
    
    /* 验证和调整配置 */
    if (queue->config.initial_capacity == 0) {
        queue->config.initial_capacity = 16;
    }
    if (queue->config.resize_factor < 2) {
        queue->config.resize_factor = 2;
    }
    
    /* 分配数据数组 */
    queue->capacity = queue->config.initial_capacity;
    queue->data = malloc(sizeof(void *) * queue->capacity);
    if (!queue->data) {
        free(queue);
        return NULL;
    }
    
    /* 初始化同步对象 */
    if (queue->config.thread_safe) {
        if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
            free(queue->data);
            free(queue);
            return NULL;
        }
        
        if (queue->config.use_condition) {
            if (pthread_cond_init(&queue->not_empty, NULL) != 0 ||
                pthread_cond_init(&queue->not_full, NULL) != 0) {
                pthread_mutex_destroy(&queue->mutex);
                free(queue->data);
                free(queue);
                return NULL;
            }
        }
    }
    
    /* 初始化统计信息 */
    queue->stats.current_capacity = queue->capacity;
    
    return queue;
}

/**
 * @brief 销毁队列
 */
void linx_queue_destroy(linx_queue_t *queue, linx_queue_free_func_t free_func)
{
    if (!queue) return;
    
    /* 清空队列 */
    linx_queue_clear(queue, free_func);
    
    /* 销毁同步对象 */
    if (queue->config.thread_safe) {
        pthread_mutex_destroy(&queue->mutex);
        
        if (queue->config.use_condition) {
            pthread_cond_destroy(&queue->not_empty);
            pthread_cond_destroy(&queue->not_full);
        }
    }
    
    /* 释放内存 */
    free(queue->data);
    free(queue);
}

/**
 * @brief 入队操作
 */
linx_queue_result_t linx_queue_push(linx_queue_t *queue, void *data)
{
    linx_queue_result_t result = LINX_QUEUE_OK;
    
    if (!queue || !data) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
    }
    
    /* 检查队列是否已满 */
    if (queue->size >= queue->capacity) {
        /* 尝试扩容 */
        uint32_t new_capacity = queue->capacity * queue->config.resize_factor;
        result = linx_queue_resize(queue, new_capacity);
        
        if (result != LINX_QUEUE_OK) {
            if (queue->config.thread_safe) {
                pthread_mutex_unlock(&queue->mutex);
            }
            return result;
        }
    }
    
    /* 入队 */
    queue->data[queue->tail] = data;
    queue->tail = linx_queue_next_index(queue->tail, queue->capacity);
    queue->size++;
    queue->stats.current_size = queue->size;
    queue->stats.total_pushed++;
    
    /* 通知等待的消费者 */
    if (queue->config.thread_safe && queue->config.use_condition) {
        pthread_cond_signal(&queue->not_empty);
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 出队操作（非阻塞）
 */
linx_queue_result_t linx_queue_pop(linx_queue_t *queue, void **data)
{
    if (!queue || !data) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
    }
    
    if (queue->size == 0) {
        if (queue->config.thread_safe) {
            pthread_mutex_unlock(&queue->mutex);
        }
        return LINX_QUEUE_EMPTY;
    }
    
    /* 出队 */
    *data = queue->data[queue->head];
    queue->head = linx_queue_next_index(queue->head, queue->capacity);
    queue->size--;
    queue->stats.current_size = queue->size;
    queue->stats.total_popped++;
    
    /* 通知等待的生产者 */
    if (queue->config.thread_safe && queue->config.use_condition) {
        pthread_cond_signal(&queue->not_full);
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 出队操作（带超时）
 */
linx_queue_result_t linx_queue_pop_timeout(linx_queue_t *queue, void **data, int timeout_ms)
{
    struct timespec abs_timeout;
    struct timeval now;
    int ret;
    
    if (!queue || !data) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (!queue->config.thread_safe || !queue->config.use_condition) {
        /* 不支持阻塞操作，降级为非阻塞 */
        return linx_queue_pop(queue, data);
    }
    
    /* 超时时间为0，非阻塞模式 */
    if (timeout_ms == 0) {
        return linx_queue_pop(queue, data);
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* 计算绝对超时时间 */
    if (timeout_ms > 0) {
        gettimeofday(&now, NULL);
        abs_timeout.tv_sec = now.tv_sec + timeout_ms / 1000;
        abs_timeout.tv_nsec = (now.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
        
        if (abs_timeout.tv_nsec >= 1000000000) {
            abs_timeout.tv_sec++;
            abs_timeout.tv_nsec -= 1000000000;
        }
    }
    
    /* 等待队列非空 */
    while (queue->size == 0 && !queue->interrupted) {
        if (timeout_ms < 0) {
            /* 永久等待 */
            ret = pthread_cond_wait(&queue->not_empty, &queue->mutex);
        } else {
            /* 带超时等待 */
            ret = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &abs_timeout);
        }
        
        if (ret == ETIMEDOUT) {
            queue->stats.total_timeouts++;
            pthread_mutex_unlock(&queue->mutex);
            return LINX_QUEUE_TIMEOUT;
        } else if (ret != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return LINX_QUEUE_ERROR;
        }
    }
    
    /* 检查中断标志 */
    if (queue->interrupted) {
        pthread_mutex_unlock(&queue->mutex);
        return LINX_QUEUE_INTERRUPTED;
    }
    
    /* 出队 */
    *data = queue->data[queue->head];
    queue->head = linx_queue_next_index(queue->head, queue->capacity);
    queue->size--;
    queue->stats.current_size = queue->size;
    queue->stats.total_popped++;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 查看队头元素
 */
linx_queue_result_t linx_queue_peek(linx_queue_t *queue, void **data)
{
    if (!queue || !data) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
    }
    
    if (queue->size == 0) {
        if (queue->config.thread_safe) {
            pthread_mutex_unlock(&queue->mutex);
        }
        return LINX_QUEUE_EMPTY;
    }
    
    *data = queue->data[queue->head];
    
    if (queue->config.thread_safe) {
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 获取队列大小
 */
int linx_queue_size(linx_queue_t *queue)
{
    int size;
    
    if (!queue) {
        return -1;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
        size = queue->size;
        pthread_mutex_unlock(&queue->mutex);
    } else {
        size = queue->size;
    }
    
    return size;
}

/**
 * @brief 检查队列是否为空
 */
bool linx_queue_is_empty(linx_queue_t *queue)
{
    return linx_queue_size(queue) == 0;
}

/**
 * @brief 检查队列是否已满
 */
bool linx_queue_is_full(linx_queue_t *queue)
{
    bool is_full;
    
    if (!queue) {
        return true;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
    }
    
    if (queue->config.auto_resize) {
        /* 自动扩容的队列只有在达到最大容量限制时才算满 */
        is_full = (queue->config.max_capacity > 0) && 
                  (queue->size >= queue->config.max_capacity);
    } else {
        is_full = (queue->size >= queue->capacity);
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return is_full;
}

/**
 * @brief 清空队列
 */
linx_queue_result_t linx_queue_clear(linx_queue_t *queue, linx_queue_free_func_t free_func)
{
    void *data;
    
    if (!queue) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
    }
    
    /* 释放所有元素 */
    if (free_func) {
        while (queue->size > 0) {
            data = queue->data[queue->head];
            queue->head = linx_queue_next_index(queue->head, queue->capacity);
            queue->size--;
            free_func(data);
        }
    } else {
        queue->head = 0;
        queue->tail = 0;
        queue->size = 0;
    }
    
    queue->stats.current_size = queue->size;
    
    if (queue->config.thread_safe) {
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 获取统计信息
 */
linx_queue_result_t linx_queue_get_stats(linx_queue_t *queue, linx_queue_stats_t *stats)
{
    if (!queue || !stats) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe) {
        pthread_mutex_lock(&queue->mutex);
        *stats = queue->stats;
        stats->current_size = queue->size;
        stats->current_capacity = queue->capacity;
        pthread_mutex_unlock(&queue->mutex);
    } else {
        *stats = queue->stats;
        stats->current_size = queue->size;
        stats->current_capacity = queue->capacity;
    }
    
    return LINX_QUEUE_OK;
}

/**
 * @brief 中断所有等待操作
 */
linx_queue_result_t linx_queue_interrupt_all(linx_queue_t *queue)
{
    if (!queue) {
        return LINX_QUEUE_INVALID_ARG;
    }
    
    if (queue->config.thread_safe && queue->config.use_condition) {
        pthread_mutex_lock(&queue->mutex);
        queue->interrupted = true;
        pthread_cond_broadcast(&queue->not_empty);
        pthread_cond_broadcast(&queue->not_full);
        pthread_mutex_unlock(&queue->mutex);
    }
    
    return LINX_QUEUE_OK;
}