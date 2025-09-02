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
#include "linx_rule_engine_match.h"
#include "linx_rule_engine_set.h"
#include "rule_match_mt.h"
#include "linx_event_processor.h"
#include "linx_config.h"
#include "linx_resource_cleanup.h"
#include "linx_event_queue.h"
#include "linx_event.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_control.h"

static int linx_event_loop(void)
{
    int ret = 0;
    linx_event_t *event = NULL;
    linx_config_t *config = linx_get_config();
    int64_t fd = -1;  /* 默认fd值 */

    ret = linx_engine_start();
    if (ret) {
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
            /* 非致命错误 */
        }

        /* 根据配置选择单线程或多线程规则匹配 */
        if (config && config->mt_config.enable_mt_match) {
            /* 已由 linx_event_processor 进行多线程调度，这里保持单线程匹配接口 */
            ret = linx_rule_set_match_rule();
        } else {
            ret = linx_rule_set_match_rule();
        }
        
        if (ret) {
            /* 匹配成功 */
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

    /**
     * 注册信号，收到信号进行资源回收操作
    */
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

    ret = linx_control_create(NULL);
    if (ret) {
        fprintf(stderr, "linx_control_create failed\n");
        goto out;
    } else {
        *type = LINX_RESOURCE_CLEANUP_CONTROL;
    }

    /**
     * hash表初始化
    */
    ret = linx_hash_map_init();
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_init failed\n");
    } else {
        *type = LINX_RESOURCE_CLEANUP_HASH_MAP;
    }

    /**
     * 机器状态初始化
    */
    ret = linx_machine_status_init();
    if (ret) {
        LINX_LOG_ERROR("linx_machine_status_init failed\n");
    } else {
        *type = LINX_RESOURCE_CLEANUP_MACHINE_STATUS;
    }

    /**
     * 进程缓存初始化
    */
    ret = linx_process_cache_init();
    if (ret) {
        LINX_LOG_ERROR("linx_process_cache_init failed\n");
    } else {
        *type = LINX_RESOURCE_CLEANUP_PROCESS_CACHE;
    }

    ret = linx_event_queue_init(2);
    if (ret) {
        LINX_LOG_ERROR("linx_event_queue_init failed\n");
    } else {
        *type = LINX_RESOURCE_CLEANUP_EVENT_QUEUE;
    }

    ret = linx_event_rich_init();
    if (ret) {

    } else {
        *type = LINX_RESOURCE_CLEANUP_EVENT_RICH;
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
    
    /* 初始化多线程规则匹配（如果启用） + 事件处理器 */
    linx_config_t *config = linx_get_config();
    if (config && config->mt_config.enable_mt_match) {
        ret = linx_rule_match_mt_init(config->mt_config.num_match_threads);
        if (ret) {
            LINX_LOG_ERROR("linx_rule_match_mt_init failed");
            /* 非致命错误，回退到单线程模式 */
            config->mt_config.enable_mt_match = false;
        } else {
            LINX_LOG_INFO("Initialized multi-thread rule matching with %d threads", 
                         config->mt_config.num_match_threads);

            linx_event_processor_config_t ep_cfg = {
                .fetcher_thread_count = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN),
                .matcher_thread_count = (uint32_t)(sysconf(_SC_NPROCESSORS_ONLN) * 2),
            };
            if (linx_event_processor_init(&ep_cfg) == 0) {
                linx_event_processor_start();
            }
        }
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
