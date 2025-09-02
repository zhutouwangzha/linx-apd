#include "linx_log.h"
#include "linx_alert.h"
#include "linx_config.h"
#include "linx_signal.h"
#include "linx_thread_pool.h"
#include "linx_event_table.h"
#include "linx_engine.h"
#include "linx_hash_map.h"
#include "linx_event_rich.h"
#include "linx_arg_parser.h"
#include "linx_event_queue.h"
#include "linx_rule_engine_load.h"
#include "linx_rule_engine_match.h"
#include "linx_rule_engine_set.h"
#include "rule_match_mt.h"
#include "linx_resource_cleanup.h"
#include "linx_process_cache.h"
#include "linx_machine_status.h"
#include "linx_control.h"
#include "linx_event_processor.h"
#include "linx_apd_config.h"

static linx_resource_cleanup_type_t linx_resource_cleanup_type = LINX_RESOURCE_CLEANUP_ERROR;

linx_resource_cleanup_type_t *linx_resource_cleanup_get(void)
{
    return &linx_resource_cleanup_type;
}

void linx_resource_cleanup(void)
{
    switch (linx_resource_cleanup_type) {
    case LINX_RESOURCE_CLEANUP_ENGINE:
        linx_engine_cleanup();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_RULE_ENGINE:
        linx_event_processor_deinit();  /* 清理事件处理器 */
        linx_rule_match_mt_deinit();    /* 清理多线程规则匹配 */
        linx_rule_set_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_ALERT:
        linx_alert_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_EVENT_RICH:
        linx_event_rich_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_EVENT_QUEUE:
        linx_event_queue_free();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_PROCESS_CACHE:
        linx_process_cache_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_MACHINE_STATUS:
        linx_machine_status_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_HASH_MAP:
        linx_hash_map_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_CONTROL:
        linx_control_destroy(NULL);
        /* fall through */
    case LINX_RESOURCE_CLEANUP_LOG:
        linx_log_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_CONFIG:
        linx_config_deinit();
        linx_apd_config_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_ARGS:
        linx_arg_deinit();
        break;
    default:
        break;
    }
}
