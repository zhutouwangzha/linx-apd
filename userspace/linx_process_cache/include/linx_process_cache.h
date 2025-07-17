#ifndef __LINX_PROCESS_CACHE_H__
#define __LINX_PROCESS_CACHE_H__ 

#include "linx_process_cache_node.h"

typedef struct {
    size_t capacity;
    size_t size;
    linx_process_node_t *head;
    linx_process_node_t *tail;
    time_t max_age;             /* 最大缓存时间 */
} linx_process_cache_t;

linx_process_cache_t *linx_process_cache_init(size_t capacity, time_t max_age);

void linx_process_cache_free(linx_process_cache_t *cache);

linx_process_info_t *linx_process_cache_get(linx_process_cache_t *cache, pid_t pid);

int linx_process_cache_update(linx_process_cache_t *cache,  pid_t pid);

#endif /* __LINX_PROCESS_CACHE_H__ */