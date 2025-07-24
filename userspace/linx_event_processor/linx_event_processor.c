#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "linx_event_processor_task.h"
#include "linx_log.h"
#include "linx_event.h"
#include "linx_rule_engine_set.h"

static linx_event_processor_t *g_event_processor = NULL;

static uint32_t get_cpu_count(void)
{
    return (uint32_t)get_nprocs();
}

static int linx_event_processor_validate_config(linx_event_processor_config_t *config)
{
    if (!config) {
        return -1;
    }

    if (config->fetcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||
        config->fetcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS)
    {
        return -1;
    }

    if (config->matcher_thread_count < LINX_EVENT_PROCESSOR_MIN_THREADS ||)
        config->matcher_thread_count > LINX_EVENT_PROCESSOR_MAX_THREADS)
    {
        return -1;
    }

    return 0;
}

static void linx_event_processor_get_default_config(linx_event_processor_config_t *config)
{
    uint32_t cpu_count;

    if (!config) {
        return;
    }

    memset(config, 0, sizeof(linx_event_processor_config_t));

    cpu_count = get_cpu_count();

    config->fetcher_thread_count = cpu_count;
    config->matcher_thread_count = cpu_count * 2;
}

static void *event_match_worker(void *arg, int *should_stop)
{
    linx_event_processor_task_t *task = (linx_event_processor_task_t *)arg;
    linx_event_processor_t *processor = task->processor;
    
    linx_rule_set_match_rule();

    free(task);
    return NULL;
}

static void *event_fetch_worker(void *arg, int *should_stop)
{
    linx_event_processor_task_t *match_task;
    linx_event_processor_task_t *task = (linx_event_processor_task_t *)arg;
    linx_event_processor_t *processor = task->processor;
    linx_event_t *event;
    int ret;

    while (!*should_stop) {
        ret = linx_engine_next(&event);
        if (ret <= 0) {
            usleep(1000);
            continue;
        }

        match_task = malloc(sizeof(linx_event_processor_task_t));
        if (!match_task) {
            LINX_LOG_WARNING("Failed to allocate memory for match task");
            continue;
        }

        match_task->type = LINX_TASK_TYPE_MATCH_EVENT;
        match_task->processor = processor;
        match_task->worker_id = task->worker_id;

        ret = linx_thread_pool_add_task(processor->matcher_pool, event_mathc_worker, match_task);
        if (ret) {
            LINX_LOG_WARNING("Failed to add task to matcher pool");
            free(match_task);
            continue;
        }
    }
    
    return NULL;
}

int linx_event_processor_init(linx_event_processor_config_t *config)
{
    if (g_event_processor) {
        return 0;
    }

    g_event_processor = calloc(1, sizeof(linx_event_processor_t));
    if (!g_event_processor) {
        return -1;
    }

    if (config) {
        if (linx_event_processor_validate_config(config)) {
            free(g_event_processor);
            return -1;
        }

        g_event_processor->config = *config;
    } else {
        linx_event_processor_get_default_config(&g_event_processor->config);
    }

    g_event_processor->fetcher_pool = linx_thread_pool_create(g_event_processor->config.fetcher_thread_count);
    if (!g_event_processor->fetcher_pool) {
        free(g_event_processor);
        return -1;
    }

    g_event_processor->matcher_pool = linx_thread_pool_create(g_event_processor->config.matcher_thread_count);
    if (!g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 0);
        free(g_event_processor);
        return -1;
    }

    return 0;
}

void linx_event_processor_deinit(void)
{
    if (!g_event_processor) {
        return;
    }

    if (g_event_processor->fetcher_pool) {
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 1);
    }

    if (g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);
    }

    free(g_event_processor);
    g_event_processor = NULL;
}

int linx_event_processor_start(void)
{
    linx_event_processor_task_t *task_arg;
    int ret;

    if (!g_event_processor) {
        return -1;
    }

    for (uint32_t i = 0; i < g_event_processor->config.fetcher_thread_count; i++) {
        task_arg = malloc(sizeof(linx_event_processor_task_t));
        if (!task_arg) {
            return -1;
        }

        task_arg->type = LINX_TASK_TYPE_FETCH_EVENT;
        task_arg->processor = g_event_processor;
        task_arg->worker_id = i;

        ret = linx_thread_pool_add_task(g_event_processor->fetcher_pool, event_fetch_worker, task_arg);
        if (ret) {
            free(task_arg);
            return -1;
        }
    }

    return 0;
}

int linx_event_processor_stop(void)
{
    if (!g_event_processor) {
        return -1;
    }

    if (g_event_processor->fetcher_pool) {
        linx_thread_pool_destroy(g_event_processor->fetcher_pool, 1);
    }

    if (g_event_processor->matcher_pool) {
        linx_thread_pool_destroy(g_event_processor->matcher_pool, 1);
    }

    return 0;
}
