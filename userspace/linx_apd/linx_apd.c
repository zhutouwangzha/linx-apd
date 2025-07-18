#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
#include "linx_rule_engine_match.h"
#include "linx_rule_engine_set.h"
#include "linx_resource_cleanup.h"
#include "linx_event_queue.h"
#include "linx_event_processor.h"

static int linx_event_loop(void)
{
    int ret = 0;
    linx_resource_cleanup_type_t *type = linx_resource_cleanup_get();
    linx_event_processor_config_t processor_config;
    uint64_t total_events, processed_events, matched_events;
    uint64_t last_total = 0, last_processed = 0, last_matched = 0;
    int stats_counter = 0;

    ret = linx_event_rich_init();
    if (ret) {
        LINX_LOG_ERROR("linx_event_rich_init failed");
        return ret;
    } else {
        *type = LINX_RESOURCE_CLEANUP_EVENT_RICH;
    }

    ret = linx_engine_start();
    if (ret) {
        LINX_LOG_ERROR("linx_engine_start failed");
        return ret;
    }

    /* 配置事件处理器 */
    memset(&processor_config, 0, sizeof(processor_config));
    processor_config.event_fetcher_threads = 0;    /* 使用默认值(CPU核数) */
    processor_config.rule_matcher_threads = 0;     /* 使用默认值(CPU核数*2) */
    processor_config.event_queue_size = 1000;
    processor_config.enriched_queue_size = 2000;

    /* 初始化事件处理器 */
    ret = linx_event_processor_init(&processor_config);
    if (ret) {
        LINX_LOG_ERROR("linx_event_processor_init failed");
        return ret;
    }

    /* 启动事件处理器 */
    ret = linx_event_processor_start();
    if (ret) {
        LINX_LOG_ERROR("linx_event_processor_start failed");
        return ret;
    } else {
        *type = LINX_RESOURCE_CLEANUP_EVENT_PROCESSOR;
    }

    LINX_LOG_INFO("Multi-threaded event processing started");

    /* 主循环 - 监控和统计 */
    while (1) {
        sleep(5); /* 每5秒输出一次统计信息 */
        
        /* 获取统计信息 */
        linx_event_processor_get_stats(&total_events, &processed_events, &matched_events);
        
        /* 计算增量 */
        uint64_t delta_total = total_events - last_total;
        uint64_t delta_processed = processed_events - last_processed;
        uint64_t delta_matched = matched_events - last_matched;
        
        LINX_LOG_INFO("Event Stats: Total=%lu (+%lu), Processed=%lu (+%lu), Matched=%lu (+%lu)",
                      total_events, delta_total, processed_events, delta_processed, 
                      matched_events, delta_matched);
        
        last_total = total_events;
        last_processed = processed_events;
        last_matched = matched_events;
        
        stats_counter++;
        
        /* 每分钟输出详细统计 */
        if (stats_counter % 12 == 0) {
            LINX_LOG_INFO("=== Event Processing Performance ===");
            LINX_LOG_INFO("Total Events: %lu", total_events);
            LINX_LOG_INFO("Processed Events: %lu", processed_events);
            LINX_LOG_INFO("Matched Events: %lu", matched_events);
            if (total_events > 0) {
                LINX_LOG_INFO("Processing Rate: %.2f%%", 
                             (double)processed_events / total_events * 100);
                LINX_LOG_INFO("Match Rate: %.2f%%", 
                             (double)matched_events / processed_events * 100);
            }
            LINX_LOG_INFO("=====================================");
        }
    }

    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    linx_arg_config_t *linx_arg_config;
    linx_global_config_t *linx_global_config;
    const struct argp *linx_argp = linx_argp_get_argp();
    linx_resource_cleanup_type_t *type = linx_resource_cleanup_get();

    /* 注册信号 */
    linx_setup_signal(SIGINT);
    linx_setup_signal(SIGUSR1);

    /* 参数解析 */
    ret = linx_arg_init();
    if (ret) {
        fprintf(stderr, "linx_arg_init failed\n");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_ARGS;
    }

    ret = argp_parse(linx_argp, argc, argv, 0, 0, 0);
    if (ret) {
        fprintf(stderr, "argp_parse failed\n");
        goto out;
    } else {
        linx_arg_config = linx_arg_get_config();
    }

    /* yaml 配置加载 */
    ret = linx_config_init();
    if (ret) {
        fprintf(stderr, "linx_config_init failed\n");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_CONFIG;
    }

    ret = linx_config_load(linx_arg_config->linx_apd_config);
    if (ret) {
        fprintf(stderr, "linx_config_load failed\n");
        goto out;
    } else {
        linx_global_config = linx_config_get();
    }

    /**
     * 日志初始化
     * 计划在 linx_apd.yaml中也有日志的相关配置
     * 所以在 linx_config_load 后初始化
     * 后续应该将 linx_global_config 某个成员传入该函数
    */
    ret = linx_log_init(linx_global_config->log_config.output, linx_global_config->log_config.log_level);
    if (ret) {
        fprintf(stderr, "linx_log_init failed\n");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_LOG;
    }

    ret = linx_event_queue_init(2);
    if (ret) {
        LINX_LOG_ERROR("linx_event_queue_init failed\n");
    }

    /**
     * 告警模块初始化
     * 后续应该将 linx_global_config 某个成员传入该函数
    */
    ret = linx_alert_init(4);
    if (ret) {
        LINX_LOG_ERROR("linx_alert_init failed");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_ALERT;
    }

    /* yaml 规则加载 */
    ret = linx_rule_engine_load(linx_arg_config->linx_apd_rules);
    if (ret) {
        LINX_LOG_ERROR("linx_rule_engine_load failed");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_RULE_ENGINE;
    }

    /* 根据配置初始化采集模块 */
    ret = linx_engine_init(linx_global_config);
    if (ret) {
        LINX_LOG_ERROR("linx_engine_init failed");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_ENGINE;
    }

    /* 启动事件循环，采集数据 */
    ret = linx_event_loop();
    if (ret) {
        LINX_LOG_ERROR("linx_event_loop failed");
    }

out:
    raise(SIGUSR1);
    return ret;
}
