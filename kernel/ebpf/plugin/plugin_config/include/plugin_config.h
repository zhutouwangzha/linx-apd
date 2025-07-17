#ifndef __PLUGIN_CONFIG_H__
#define __PLUGIN_CONFIG_H__

#include <stdint.h>

#include "plugin_config_common.h"
#include "linx_size_define.h"
#include "linx_syscall_id.h"

#include "macro/plugin_init_config_macro.h"
#include "macro/plugin_open_config_macro.h"

#define PLUGIN_INIT_CONFIG_MACRO(num, up_name, ...)    \
    INIT_IDX_##up_name = num,

typedef enum {
    PLUGIN_INIT_CONFIG_MACRO_ALL
    INIT_IDX_MAX
} init_config_idx_t;

#undef PLUGIN_INIT_CONFIG_MACRO

#define PLUGIN_OPEN_CONFIG_MACRO(num, up_name, ...)    \
    OPEN_IDX_##up_name = num,

typedef enum {
    PLUGIN_OPEN_CONFIG_MACRO_ALL
    OPEN_IDX_MAX
} open_config_idx_t;

#undef PLUGIN_OPEN_CONFIG_MACRO

/**
 * 该配置主要用于控制插件的执行状态
 */
typedef struct {
    uint64_t    rich_value_size;            /* 指定丰富事件信息的最大Buffer长度 */
    uint64_t    str_max_size;               /* 指定输出系统调用参数中字符串的最大长度 */
    uint8_t     log_level;                  /* 指定linx_log的输出日志等级 */
    char        log_file[LINX_PATH_MAX_SIZE];/* 指定linx_log输出文件路径 */

    struct {
        char    path[LINX_PATH_MAX_SIZE];   /* 输出文件路径 */
        uint8_t format;                     /* 输出格式，计划支持json和normal */
        uint8_t put_flag;                   /* 标记是否写到文件 */
        int     fd;                         /* 文件对应的fd */
    } output_config;
} init_config_t;

/**
 * 该配置主要用于控制ebpf程序的执行状态
 */
typedef struct {
    uint8_t     filter_own;             /* 在bpf采集过程中，是否过滤插件自身的系统调用 */
    uint8_t     filter_falco;           /* 在bpf采集过程中，是否过滤falco的系统调用 */
    uint8_t     drop_mode;              /* 在bpf采集过程中，放弃所有采集的总开关 */
    uint8_t     drop_failed;            /* 在bpf采集过程中，放弃采集失败的系统调用 */
    uint32_t    filter_pids[LINX_BPF_FILTER_PID_MAX_SIZE]; /* bpf采集过程中，要过滤掉的pid */
    uint8_t     filter_comms[LINX_BPF_FILTER_COMM_MAX_SIZE][LINX_COMM_MAX_SIZE];    /* bpf采集过程中，要过滤的命令 */
    uint8_t     interest_syscall_table[LINX_SYSCALL_MAX_IDX];  /* 存放要采集的系统调用 */
} open_config_t;

typedef struct {
    init_config_t init_config;
    open_config_t open_config;
} plugin_config_t;

extern plugin_config_t g_plugin_config;

void plugin_config_parse_init_config(const char *config);

void plugin_config_parse_open_config(const char *config);

#endif /* __PLUGIN_CONFIG_H__ */
