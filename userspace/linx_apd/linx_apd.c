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
#include "linx_resource_cleanup.h"
#include "linx_event_queue.h"
#include "linx_event.h"
static int linx_event_loop(void)
{
    int ret = 0;
    linx_resource_cleanup_type_t *type = linx_resource_cleanup_get();
    linx_event_t *event = NULL;

    ret = linx_engine_start();
    if (ret) {

    }

    while (1) {
        ret = linx_engine_next(&event);
        if (ret <= 0) {
            continue;
        }

        ret = linx_event_rich(event);
        if (ret) {

        }

        ret = linx_event_queue_push();
        if (ret) {

        }

        ret = linx_rule_set_match_rule();
        if (ret) {

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
