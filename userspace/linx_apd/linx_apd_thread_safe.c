#include <stdio.h>
#include <signal.h>

#include "linx_log.h"
#include "linx_alert.h"
#include "linx_config.h"
#include "linx_signal.h"
#include "linx_thread_pool.h"
#include "linx_event_table.h"
#include "linx_engine.h"
#include "linx_event_rich.h"
#include "linx_arg_parser.h"
#include "linx_event_queue.h"
#include "linx_rule_engine_load.h"
#include "linx_rule_engine_set.h"
#include "linx_resource_cleanup.h"
#include "linx_event_queue.h"
#include "linx_event.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_control.h"
#include "linx_hash_map_thread_safe.h"
#include "linx_rule_engine_set_thread_safe.h"
#include "linx_event_processor.h"

/**
 * 多线程版本的事件循环
 */
static int linx_event_loop_multi_thread(void)
{
    int ret = 0;
    linx_event_processor_config_t config;
    linx_event_processor_t *processor;
    
    /* 初始化线程安全的hash map */
    ret = linx_hash_map_thread_safe_init();
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_thread_safe_init failed");
        return ret;
    }
    
    /* 获取默认的事件处理器配置 */
    linx_event_processor_get_default_config(&config);
    
    /* 创建事件处理器 */
    ret = linx_event_processor_create(&processor, &config);
    if (ret) {
        LINX_LOG_ERROR("linx_event_processor_create failed");
        goto cleanup;
    }
    
    /* 启动事件处理器 */
    ret = linx_event_processor_start(processor);
    if (ret) {
        LINX_LOG_ERROR("linx_event_processor_start failed");
        goto cleanup;
    }
    
    LINX_LOG_INFO("Multi-threaded event processing started with %u fetcher threads and %u matcher threads",
                  config.fetcher_thread_count, config.matcher_thread_count);
    
    /* 等待处理器运行 */
    ret = linx_event_processor_wait(processor);
    
cleanup:
    if (processor) {
        linx_event_processor_stop(processor);
        linx_event_processor_destroy(processor);
    }
    
    linx_hash_map_thread_safe_deinit();
    
    return ret;
}

/**
 * 单线程版本的事件循环（保持兼容性）
 */
static int linx_event_loop_single_thread(void)
{
    int ret = 0;
    linx_event_t *event = NULL;

    ret = linx_engine_start();
    if (ret) {
        LINX_LOG_ERROR("linx_engine_start failed");
        return ret;
    }

    while (1) {
        ret = linx_engine_next(&event);
        if (ret <= 0) {
            continue;
        }

        ret = linx_event_rich(event);
        if (ret) {
            continue;
        }

        ret = linx_event_queue_push();
        if (ret) {
            LINX_LOG_WARNING("linx_event_queue_push failed");
        }

        ret = linx_rule_set_match_rule();
        if (ret) {
            LINX_LOG_DEBUG("Rule matched");
        }
    }

    return ret;
}

/**
 * 主事件循环入口，根据配置选择单线程或多线程模式
 */
int linx_event_loop_with_mode(bool multi_thread)
{
    if (multi_thread) {
        LINX_LOG_INFO("Starting multi-threaded event processing mode");
        return linx_event_loop_multi_thread();
    } else {
        LINX_LOG_INFO("Starting single-threaded event processing mode");
        return linx_event_loop_single_thread();
    }
}