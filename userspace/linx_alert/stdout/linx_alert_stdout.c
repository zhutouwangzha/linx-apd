#include <stdio.h>
#include <time.h>
#include <string.h>

#include "linx_alert.h"

/* ANSI 颜色代码 */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
 * @brief 根据优先级获取颜色代码
 * @param priority 优先级
 * @return 颜色代码字符串
 */
static const char *get_priority_color(int priority)
{
    switch (priority) {
        case 0: return ANSI_COLOR_RED;      // 紧急
        case 1: return ANSI_COLOR_MAGENTA;  // 严重
        case 2: return ANSI_COLOR_YELLOW;   // 警告
        case 3: return ANSI_COLOR_BLUE;     // 信息
        default: return ANSI_COLOR_RESET;
    }
}

/**
 * @brief 获取优先级名称
 * @param priority 优先级
 * @return 优先级名称字符串
 */
static const char *get_priority_name(int priority)
{
    switch (priority) {
        case 0: return "CRITICAL";
        case 1: return "ERROR";
        case 2: return "WARNING";
        case 3: return "INFO";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 格式化时间戳
 * @param timestamp 时间戳
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
static int format_timestamp(time_t timestamp, char *buffer, size_t buffer_size)
{
    struct tm *tm_info = localtime(&timestamp);
    if (!tm_info) {
        return -1;
    }

    if (strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief stdout 输出函数
 * @param message 告警消息
 * @param config 输出配置
 * @return 成功返回0，失败返回-1
 */
int linx_alert_output_stdout(linx_alert_message_t *message, linx_output_config_t *config)
{
    if (!message || !config || config->type != LINX_OUTPUT_TYPE_STDOUT) {
        return -1;
    }

    FILE *output = stdout;
    
    // 是否使用颜色
    bool use_color = config->config.stdout_config.use_color;
    
    // 是否显示时间戳
    if (config->config.stdout_config.timestamp) {
        char timestamp_buf[64];
        if (format_timestamp(message->timestamp, timestamp_buf, sizeof(timestamp_buf)) == 0) {
            if (use_color) {
                fprintf(output, "%s[%s]%s ", ANSI_COLOR_CYAN, timestamp_buf, ANSI_COLOR_RESET);
            } else {
                fprintf(output, "[%s] ", timestamp_buf);
            }
        }
    }

    // 显示优先级
    if (use_color) {
        fprintf(output, "%s[%s]%s ", 
                get_priority_color(message->priority), 
                get_priority_name(message->priority), 
                ANSI_COLOR_RESET);
    } else {
        fprintf(output, "[%s] ", get_priority_name(message->priority));
    }

    // 显示规则名称
    if (message->rule_name) {
        if (use_color) {
            fprintf(output, "%s[%s]%s ", ANSI_COLOR_GREEN, message->rule_name, ANSI_COLOR_RESET);
        } else {
            fprintf(output, "[%s] ", message->rule_name);
        }
    }

    // 显示消息内容
    fprintf(output, "%s\n", message->formatted_message);
    
    // 强制刷新输出
    fflush(output);

    return 0;
}