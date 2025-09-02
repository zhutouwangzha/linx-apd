#ifndef __LINX_EVENT_HANDLER_H__
#define __LINX_EVENT_HANDLER_H__

#include "linx_event_get.h"

/* 处理单个事件，包括事件丰富和规则匹配 */
int linx_handle_event(linx_event_t *event);

/* 初始化事件处理器 */
int linx_event_handler_init(void);

/* 清理事件处理器 */
void linx_event_handler_deinit(void);

#endif /* __LINX_EVENT_HANDLER_H__ */