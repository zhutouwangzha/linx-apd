#ifndef __PLUGIN_STRUCT_H__
#define __PLUGIN_STRUCT_H__

#include "scap.h"
#include "linx_ebpf_api.h"
#include "linx_event.h"
#include "linx_event_rich.h"

/**
 * Falco 消息头部的长度
 */
#define PLUGIN_EVENT_HEADER_SIZE    (sizeof(ss_plugin_event))

/**
 * 插件事件源存放参数长度区域的长度
 */
#define PLUGIN_PARAMS_SIZE_SIZE     (sizeof(plugin_params_size_t))

/**
 * 在插件事件源中所能传递消息的最大长度
 * 
 * 头部 + 参数长度区域 + 插件ID + 自定义事件长度
 */
#define PLUGIN_EVENT_MAX_SIZE       (PLUGIN_EVENT_HEADER_SIZE + \
                                     PLUGIN_PARAMS_SIZE_SIZE + \
                                     sizeof(uint32_t) + \
                                     LINX_EVENT_MAX_SIZE)

/**
 * 插件最后一次错误的最大长度
 */
#define PLUGIN_LASTERR_MAXSIZE      SCAP_LASTERR_SIZE

/**
 * 插件进入事件只允许有两个参数
 * 1 - 插件ID
 * 2 - 插件返回的消息
 */
#define PLUGIN_NPARAMS              (2)

/* 插件ID */
#define PLUGIN_ID                   (999U)

typedef struct {
    uint32_t params_size[PLUGIN_NPARAMS];
} plugin_params_size_t;

typedef struct {
    char *lasterr;
    ss_plugin_owner_t *owner;
    ss_plugin_log_fn_t log;
    linx_event_rich_t rich_value;
    char *comm_store;
    char *cmdline_store;
    char *fullpath_store;
} plugin_init_state_t;

typedef struct {
    ss_plugin_event *evt;
    linx_ebpf_t bpf_manager;
} plugin_open_state_t;

#endif /* __PLUGIN_STRUCT_H__ */
