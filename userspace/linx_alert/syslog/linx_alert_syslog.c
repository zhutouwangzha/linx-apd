#include <stdio.h>
#include <syslog.h>
#include <string.h>

#include "linx_alert.h"

/**
 * @brief 将优先级转换为 syslog 优先级
 * @param priority 内部优先级
 * @param config_priority 配置的 syslog 优先级（可选）
 * @return syslog 优先级
 */
static int get_syslog_priority(int priority, int config_priority)
{
    // 如果配置中指定了优先级，使用配置的优先级
    if (config_priority >= 0) {
        return config_priority;
    }

    // 否则根据内部优先级映射
    switch (priority) {
        case 0: return LOG_CRIT;    // 紧急
        case 1: return LOG_ERR;     // 错误
        case 2: return LOG_WARNING; // 警告
        case 3: return LOG_INFO;    // 信息
        default: return LOG_NOTICE; // 默认
    }
}

/**
 * @brief 将配置的 facility 转换为 syslog facility
 * @param facility_config 配置的 facility
 * @return syslog facility
 */
static int get_syslog_facility(int facility_config)
{
    switch (facility_config) {
        case 0: return LOG_KERN;     // 内核消息
        case 1: return LOG_USER;     // 用户级消息
        case 2: return LOG_MAIL;     // 邮件系统
        case 3: return LOG_DAEMON;   // 系统守护进程
        case 4: return LOG_AUTH;     // 安全/授权消息
        case 5: return LOG_SYSLOG;   // syslog 内部消息
        case 6: return LOG_LPR;      // 行式打印机子系统
        case 7: return LOG_NEWS;     // 网络新闻子系统
        case 8: return LOG_UUCP;     // UUCP 子系统
        case 9: return LOG_CRON;     // 时钟守护进程
        case 10: return LOG_AUTHPRIV; // 安全/授权消息（私有）
        case 11: return LOG_FTP;     // FTP 守护进程
        case 16: return LOG_LOCAL0;  // 本地使用 0
        case 17: return LOG_LOCAL1;  // 本地使用 1
        case 18: return LOG_LOCAL2;  // 本地使用 2
        case 19: return LOG_LOCAL3;  // 本地使用 3
        case 20: return LOG_LOCAL4;  // 本地使用 4
        case 21: return LOG_LOCAL5;  // 本地使用 5
        case 22: return LOG_LOCAL6;  // 本地使用 6
        case 23: return LOG_LOCAL7;  // 本地使用 7
        default: return LOG_USER;    // 默认使用用户级
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
 * @brief syslog 输出函数
 * @param message 告警消息
 * @param config 输出配置
 * @return 成功返回0，失败返回-1
 */
int linx_alert_output_syslog(linx_alert_message_t *message, linx_output_config_t *config)
{
    if (!message || !config || config->type != LINX_OUTPUT_TYPE_SYSLOG) {
        return -1;
    }

    // 获取 syslog facility 和 priority
    int facility = get_syslog_facility(config->config.syslog_config.facility);
    int priority = get_syslog_priority(message->priority, config->config.syslog_config.priority);

    // 打开 syslog 连接（如果尚未打开）
    // 使用 LOG_PID 选项包含进程 ID，LOG_CONS 选项在无法写入时输出到控制台
    openlog("linx", LOG_PID | LOG_CONS, facility);

    // 构建日志消息
    char log_message[2048];
    
    if (message->rule_name) {
        snprintf(log_message, sizeof(log_message), 
                "[%s] [%s] %s", 
                get_priority_name(message->priority),
                message->rule_name,
                message->formatted_message);
    } else {
        snprintf(log_message, sizeof(log_message), 
                "[%s] %s", 
                get_priority_name(message->priority),
                message->formatted_message);
    }

    // 发送到 syslog
    syslog(priority, "%s", log_message);

    // 关闭 syslog 连接
    closelog();

    return 0;
}