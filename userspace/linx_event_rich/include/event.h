#ifndef __EVENT_H__
#define __EVENT_H__ 

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t num;   /* 事件编号 */
    char time[64];  /* 时间戳字符串，包含纳秒（查看falco还有不是纳秒的情况，后续考虑用enum实现？） */
    char *type;     /* 对应的系统调用名，这里用指针是因为在linx_syscall_table中定义好了字符串，不用再拷贝一份 */
    char *args;     /* 所有事件参数聚合为一个字符串 */
    struct {
        void *data[32];
    } rawarg;               /* 事件参数之一，由名称指定，evt.rawargs.fd */
    struct {
        void *data[32];
    } arg;                  /* 事件参数之一，由名称或编号指定，evt.arg.fd，evt.arg[0] */
    char res[16];   /* 成功是SUCCESS，失败是错误码字符串 */
    int64_t rawres; /* 返回值的具体数值 */
    bool failed;    /* 返回失败的事件，该值为true */
    char dir[1];    /* > 表示进入事件，< 表示退出事件 */
} event_t;

#endif /* __EVENT_H__ */
