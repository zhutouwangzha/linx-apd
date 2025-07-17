#ifndef __EVENT_H__
#define __EVENT_H__ 

#include <stdint.h>

typedef struct {
    uint64_t time;     /* 时间戳 */
    uint64_t num;           /* 事件编号 */
    uint64_t type;          /* 事件类型,不知道用下标还是实际的字符串 */
    uint64_t cpu;           /* 触发事件的cpu编号 */
    char dir[1];            /* > 表示进入，< 表示离开 */
    /**
     * args     将所有事件参数聚合为一个字符串
     * rawarg   事件参数之一，由名称指定（如，evt.rawarg.fd）
     * arg      事件参数之一，按名称或编号指定
     * buffer   事件的二进制数据缓冲区，如read、recvfrom等
     * buflen   buffer的长度
     * res      事件返回值为字符串。如果事件失败，是FAILED，成功是SUCCESS
     * rawres   事件返回值，例如-2，用于范围比较
     * failed   对于返回错误状态的事件，返回true
    */
} event_t;

#endif /* __EVENT_H__ */
