#include "plugin_config.h"
#include "linx_common.h"
#include "linx_log.h"

/**
 * 初始化配置结构体的默认值
 * 即使配置文件错误，也能按照默认配置执行
 */
plugin_config_t g_plugin_config = {
    .init_config = {
        .rich_value_size    = 1024,
        .str_max_size       = 100,
        .output_config = {
            .fd             = -1,
            .format         = 0,
            .path           = "",
            .put_flag       = 0                         /* 默认不输出到文件 */
        },
        .log_level          = (uint8_t)LINX_LOG_ERROR,  /* 默认日志级别为ERROR */
        .log_file           = ""                        /* 默认将日志输出到 stderr */
    },
    .open_config = {
        .filter_own         = 0,                        /* 不过滤插件本身的系统调用 */
        .filter_falco       = 0,                        /* 不过滤Falco的系统调用 */
        .filter_pids        = { 0 },
        .filter_comms       = {
            {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
            {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
        },
        .drop_mode          = 0,
        .drop_failed        = 0,                        /* 不丢弃失败的系统调用 */
        .interest_syscall_table  = {
            [LINX_SYSCALL_UNLINKAT] = 1                 /* 只采集 unlinkat 系统调用 */
        }
    }
};
