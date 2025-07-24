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
#include "linx_resource_cleanup.h"
#include "linx_process_cache.h"

static linx_resource_cleanup_type_t linx_resource_cleanup_type = LINX_RESOURCE_CLEANUP_ERROR;

linx_resource_cleanup_type_t *linx_resource_cleanup_get(void)
{
    return &linx_resource_cleanup_type;
}

void linx_resource_cleanup(void)
{
    switch (linx_resource_cleanup_type) {
    case LINX_RESOURCE_CLEANUP_ENGINE:
    case LINX_RESOURCE_CLEANUP_RULE_ENGINE:
        linx_rule_set_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_ALERT:
        linx_alert_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_EVENT_RICH:
    case LINX_RESOURCE_CLEANUP_EVENT_QUEUE:
        linx_event_queue_free();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_PROCESS_CACHE:
        linx_process_cache_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_HASH_MAP:
        linx_hash_map_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_LOG:
        linx_log_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_CONFIG:
        linx_config_deinit();
        /* fall through */
    case LINX_RESOURCE_CLEANUP_ARGS:
        linx_arg_deinit();
        break;
    default:
        break;
    }
}
