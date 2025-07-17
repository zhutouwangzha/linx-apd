#include <stdlib.h>

#include "linx_process_cache.h"

linx_process_cache_t *linx_process_cache_init(size_t capacity, time_t max_age)
{
    (void)capacity;
    (void)max_age;

    return NULL;
}

void linx_process_cache_free(linx_process_cache_t *cache)
{
    (void)cache;
}

linx_process_info_t *linx_process_cache_get(linx_process_cache_t *cache, pid_t pid)
{
    (void)cache;
    (void)pid;

    return NULL;
}

int linx_process_cache_update(linx_process_cache_t *cache,  pid_t pid)
{
    (void)cache;
    (void)pid;

    return 0;
}
