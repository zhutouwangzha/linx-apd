#ifndef __LINX_EVENT_RICH_THREAD_SAFE_H__
#define __LINX_EVENT_RICH_THREAD_SAFE_H__

#include "linx_event.h"
#include "linx_event_rich.h"

/* 线程安全的事件丰富处理函数 */
int linx_event_rich_thread_safe(linx_event_t *event);

/* 获取线程本地的事件结构 */
event_t *linx_event_rich_get_thread_safe(void);

/* 线程清理函数 */
void linx_event_rich_thread_cleanup(void);

#endif /* __LINX_EVENT_RICH_THREAD_SAFE_H__ */