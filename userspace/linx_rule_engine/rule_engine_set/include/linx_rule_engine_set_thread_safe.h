#ifndef __LINX_RULE_ENGINE_SET_THREAD_SAFE_H__
#define __LINX_RULE_ENGINE_SET_THREAD_SAFE_H__

#include <stdbool.h>
#include "linx_event.h"

/* 线程安全的规则匹配函数 */
bool linx_rule_set_match_rule_thread_safe(void);

/* 多线程事件处理函数 */
int linx_event_process_thread_safe(linx_event_t *event);

/* 线程清理函数 */
void linx_rule_engine_thread_cleanup(void);

#endif /* __LINX_RULE_ENGINE_SET_THREAD_SAFE_H__ */