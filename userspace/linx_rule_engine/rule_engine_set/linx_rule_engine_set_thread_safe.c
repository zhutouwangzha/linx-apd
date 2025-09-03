#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "linx_rule_engine_set.h"
#include "linx_alert.h"
#include "linx_hash_map_thread_safe.h"
#include "linx_event_rich_thread_safe.h"
#include "linx_log.h"

/* 外部引用原有的规则集（只读访问） */
extern linx_rule_set_t *rule_set;

/* 线程安全版本的规则匹配函数 */
bool linx_rule_set_match_rule_thread_safe(void)
{
    bool match = false;
    
    if (rule_set == NULL) {
        return false;
    }
    
    /* 确保线程已注册 */
    if (linx_hash_map_register_thread() != 0) {
        LINX_LOG_ERROR("Failed to register thread for rule matching");
        return false;
    }
    
    for (size_t i = 0; i < rule_set->size; i++) {
        if (rule_set->data.matches[i]) {
            /* 使用线程安全的匹配函数 */
            if (rule_set->data.matches[i]->func_thread_safe) {
                if (rule_set->data.matches[i]->func_thread_safe(rule_set->data.matches[i]->context)) {
                    match = true;
                    
                    linx_alert_send_async(rule_set->data.outputs[i], rule_set->data.rules[i]);
                    /**
                     * 这里有一个yaml配置可以控制匹配到规则后是否继续匹配后面的规则
                     * 计划在后续添加 
                     * 这里还要添加一个匹配成功然后输出消息的逻辑
                    */
                    break;
                }
            } else if (rule_set->data.matches[i]->func) {
                /* 回退到原有的匹配函数（非线程安全） */
                LINX_LOG_WARNING("Using non-thread-safe rule matching function for rule %zu", i);
                if (rule_set->data.matches[i]->func(rule_set->data.matches[i]->context)) {
                    match = true;
                    
                    linx_alert_send_async(rule_set->data.outputs[i], rule_set->data.rules[i]);
                    break;
                }
            }
        }
    }
    
    return match;
}

/**
 * 多线程环境下的事件处理函数
 */
int linx_event_process_thread_safe(linx_event_t *event)
{
    int ret;
    
    /* 确保线程已注册 */
    if (linx_hash_map_register_thread() != 0) {
        LINX_LOG_ERROR("Failed to register thread for event processing");
        return -1;
    }
    
    /* 使用线程安全的事件丰富处理 */
    ret = linx_event_rich_thread_safe(event);
    if (ret) {
        LINX_LOG_WARNING("Thread-safe event enrichment failed");
        return ret;
    }
    
    /* 使用线程安全的规则匹配 */
    linx_rule_set_match_rule_thread_safe();
    
    return 0;
}

/**
 * 工作线程清理函数
 */
void linx_rule_engine_thread_cleanup(void)
{
    linx_event_rich_thread_cleanup();
    linx_hash_map_unregister_thread();
}