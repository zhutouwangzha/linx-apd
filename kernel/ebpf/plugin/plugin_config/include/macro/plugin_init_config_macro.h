#ifndef __PLUGIN_INIT_CONFIG_MACRO_H__
#define __PLUGIN_INIT_CONFIG_MACRO_H__

/**
 * INIT_CONFIG 主要用于控制插件的行为，例如：
 *  控制日志缓冲区的大小
 *  控制日志是否输出到文件等等
 */
#define PLUGIN_INIT_CONFIG_MACRO_ALL \
    PLUGIN_INIT_CONFIG_MACRO(0, RICH_VALUE_SIZE,    rich_value_size,    1024,       "指定丰富事件信息的最大Buffer长度") \
    PLUGIN_INIT_CONFIG_MACRO(1, STR_MAX_SIZE,       str_max_size,       1024,       "用于控制输出系统调用参数时字符串的最大长度，若实际参数大于设定值，则用...标识") \
    PLUGIN_INIT_CONFIG_MACRO(2, PUTFILE,            putfile,            "true",     "是否输出到文件") \
    PLUGIN_INIT_CONFIG_MACRO(3, PATH,               path,               "1.txt",    "输出文件路径") \
    PLUGIN_INIT_CONFIG_MACRO(4, FORMAT,             format,             "json",     "输出格式,计划支持json和normal") \
    PLUGIN_INIT_CONFIG_MACRO(5, LOG_LEVEL,          log_level,          "INFO",     "配置linx_log的日志输出等级,可选参数有'DEBUG'、'INFO'、'WARNING'、'ERROR'、'FATAL'") \
    PLUGIN_INIT_CONFIG_MACRO(6, LOG_FILE,           log_file,           "2.txt",    "配置linx_log的输出路径,为空时默认输出到stderr")

#endif /* __PLUGIN_INIT_CONFIG_MACRO_H__ */
