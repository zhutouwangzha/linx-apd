#ifndef __LINX_EVENT_CLASSIFIER_H__
#define __LINX_EVENT_CLASSIFIER_H__

#include <stdint.h>
#include "linx_event.h"
#include "linx_event_type.h"

/**
 * 事件源枚举
 * 根据系统调用的功能领域进行分类
 */
typedef enum {
    LINX_EVENT_SOURCE_FILE_IO = 0,      /* 文件I/O操作: read, write, open, close等 */
    LINX_EVENT_SOURCE_NETWORK,          /* 网络操作: socket, connect, sendto等 */
    LINX_EVENT_SOURCE_PROCESS,          /* 进程管理: fork, exec, clone等 */
    LINX_EVENT_SOURCE_MEMORY,           /* 内存管理: mmap, munmap, brk等 */
    LINX_EVENT_SOURCE_SIGNAL,           /* 信号处理: sigaction, kill等 */
    LINX_EVENT_SOURCE_IPC,              /* 进程间通信: pipe, msgget, semget等 */
    LINX_EVENT_SOURCE_FILESYSTEM,       /* 文件系统操作: stat, chmod, mkdir等 */
    LINX_EVENT_SOURCE_SECURITY,         /* 安全相关: setuid, capget等 */
    LINX_EVENT_SOURCE_SYSTEM,           /* 系统信息: uname, sysinfo等 */
    LINX_EVENT_SOURCE_TIME,             /* 时间相关: gettimeofday, nanosleep等 */
    LINX_EVENT_SOURCE_UNKNOWN,          /* 未知或其他 */
    LINX_EVENT_SOURCE_MAX
} linx_event_source_t;

/**
 * 事件方向枚举
 * 基于事件类型的后缀 _E (enter) 和 _X (exit)
 */
typedef enum {
    LINX_EVENT_DIRECTION_ENTER = 0,     /* 系统调用进入 */
    LINX_EVENT_DIRECTION_EXIT,          /* 系统调用退出 */
    LINX_EVENT_DIRECTION_MAX
} linx_event_direction_t;

/**
 * 事件分类结构
 */
typedef struct {
    linx_event_source_t source;         /* 事件源 */
    uint32_t type;                      /* 原始事件类型 */
    linx_event_direction_t direction;   /* 事件方向 */
} linx_event_classification_t;

/**
 * 初始化事件分类器
 * @return 成功返回0，失败返回-1
 */
int linx_event_classifier_init(void);

/**
 * 清理事件分类器
 */
void linx_event_classifier_deinit(void);

/**
 * 对事件进行分类
 * @param event 要分类的事件
 * @param classification 输出的分类结果
 * @return 成功返回0，失败返回-1
 */
int linx_event_classify(const linx_event_t *event, linx_event_classification_t *classification);

/**
 * 根据事件类型获取事件源
 * @param event_type 事件类型
 * @return 事件源类型
 */
linx_event_source_t linx_event_type_to_source(uint32_t event_type);

/**
 * 根据事件类型获取事件方向
 * @param event_type 事件类型
 * @return 事件方向
 */
linx_event_direction_t linx_event_type_to_direction(uint32_t event_type);

/**
 * 获取事件源的字符串表示
 * @param source 事件源
 * @return 事件源字符串
 */
const char *linx_event_source_to_string(linx_event_source_t source);

/**
 * 获取事件方向的字符串表示
 * @param direction 事件方向
 * @return 事件方向字符串
 */
const char *linx_event_direction_to_string(linx_event_direction_t direction);

#endif /* __LINX_EVENT_CLASSIFIER_H__ */