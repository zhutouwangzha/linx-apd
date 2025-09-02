#include <pthread.h>
#include "linx_event_handler.h"
#include "linx_event_rich.h"
#include "linx_event_queue.h"
#include "linx_rule_engine_set.h"
#include "rule_match_thread_context.h"
#include "linx_config.h"
#include "linx_log.h"

/* 线程本地存储，保存当前事件的fd */
static __thread int64_t tls_current_fd = -1;

/* 获取当前事件的fd */
int64_t linx_get_current_event_fd(void)
{
    return tls_current_fd;
}

/* 设置当前事件的fd */
void linx_set_current_event_fd(int64_t fd)
{
    tls_current_fd = fd;
}

/* 处理单个事件 */
int linx_handle_event(linx_event_t *event)
{
    int ret;
    linx_config_t *config = linx_get_config();
    
    /* 重置fd为默认值 */
    tls_current_fd = -1;
    
    /* 事件丰富 */
    ret = linx_event_rich(event);
    if (ret) {
        return ret;
    }
    
    /* 将事件推入队列 */
    ret = linx_event_queue_push();
    if (ret) {
        LINX_LOG_WARNING("Failed to push event to queue");
        /* 非致命错误，继续处理 */
    }
    
    /* 规则匹配 */
    if (config && config->mt_config.enable_mt_match) {
        /* 多线程规则匹配 */
        ret = linx_rule_set_match_rule_mt(event, tls_current_fd);
    } else {
        /* 单线程规则匹配 */
        ret = linx_rule_set_match_rule();
    }
    
    return ret ? 1 : 0;
}

/* 初始化事件处理器 */
int linx_event_handler_init(void)
{
    linx_config_t *config = linx_get_config();
    int ret = 0;
    
    if (config && config->mt_config.enable_mt_match) {
        thread_pool_config_t pool_config = {
            .num_threads = config->mt_config.num_match_threads,
            .queue_size = config->mt_config.match_queue_size
        };
        
        ret = linx_rule_match_thread_pool_init(&pool_config);
        if (ret) {
            LINX_LOG_ERROR("Failed to initialize rule match thread pool");
            /* 回退到单线程模式 */
            config->mt_config.enable_mt_match = false;
        } else {
            LINX_LOG_INFO("Initialized rule match thread pool with %d threads",
                         config->mt_config.num_match_threads);
        }
    }
    
    return 0;
}

/* 清理事件处理器 */
void linx_event_handler_deinit(void)
{
    linx_config_t *config = linx_get_config();
    
    if (config && config->mt_config.enable_mt_match) {
        linx_rule_match_thread_pool_destroy();
        LINX_LOG_INFO("Destroyed rule match thread pool");
    }
}