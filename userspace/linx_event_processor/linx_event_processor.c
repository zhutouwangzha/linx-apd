#include <stdlib.h>
#include <string.h>

#include "linx_event_processor.h"
#include "linx_log.h"

static linx_event_processor_t *g_event_processor = NULL;

int linx_event_processor_init(linx_event_processor_config_t *config)
{
    int cpu_count;

    if (g_event_processor) {
        return 0;
    }

    g_event_processor = malloc(sizeof(linx_event_processor_t));
    if (!g_event_processor) {
        return -1;
    }

    memset(g_event_processor, 0, sizeof(linx_event_processor_t));

    if (config) {
        g_event_processor->config = *config;
    } else {
        cpu_count = get_cpu_count();
        g_event_processor->config.event_fetcher_pool_size = cpu_count;
        g_event_processor->config.event_matcher_pool_size = cpu_count *2;
        g_event_processor->config.event_queue_size = 1000;
        g_event_processor->config.batch_size = 10;
    }

    g_event_processor->fetcher_pool = linx_thread_pool_create(
        g_event_processor->config.event_fetcher_pool_size);
    if (!g_event_processor->fetcher_pool) {
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    g_event_processor->matcher_pool = linx_thread_pool_create(
        g_event_processor->config.event_matcher_pool_size);
    if (!g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 0);
        free(g_event_processor);
        g_event_processor = NULL;
        return -1;
    }

    /* 创建事件队列 */


    if (pthread_mutex_init(&g_event_processor->status_mutex, NULL) != 0) {

    }

    g_event_processor->running = false;
    g_event_processor->shutdown = false;
    g_event_processor->last_stats_time = get_current_time_ms();

    LINX_LOG_INFO("Event processor initialized with %d fetcher threads and %d matcher threads",
                  g_event_processor->config.event_fetcher_pool_size,
                  g_event_processor->config.event_matcher_pool_size);

    return 0;
}

void linx_event_processor_deinit(void)
{

}
