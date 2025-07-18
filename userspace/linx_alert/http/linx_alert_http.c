#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "linx_alert.h"

/**
 * @brief 格式化时间戳为 ISO 8601 格式
 * @param timestamp 时间戳
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
static int format_iso8601_timestamp(time_t timestamp, char *buffer, size_t buffer_size)
{
    struct tm *tm_info = gmtime(&timestamp);
    if (!tm_info) {
        return -1;
    }

    if (strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%SZ", tm_info) == 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief 获取优先级名称
 * @param priority 优先级
 * @return 优先级名称字符串
 */
static const char *get_priority_name(int priority)
{
    switch (priority) {
        case 0: return "critical";
        case 1: return "error";
        case 2: return "warning";
        case 3: return "info";
        default: return "unknown";
    }
}

/**
 * @brief 转义 JSON 字符串
 * @param input 输入字符串
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return 成功返回0，失败返回-1
 */
static int escape_json_string(const char *input, char *output, size_t output_size)
{
    if (!input || !output || output_size == 0) {
        return -1;
    }

    size_t input_len = strlen(input);
    size_t output_pos = 0;

    for (size_t i = 0; i < input_len && output_pos < output_size - 1; i++) {
        char c = input[i];
        
        switch (c) {
            case '"':
                if (output_pos + 2 >= output_size) goto buffer_full;
                output[output_pos++] = '\\';
                output[output_pos++] = '"';
                break;
            case '\\':
                if (output_pos + 2 >= output_size) goto buffer_full;
                output[output_pos++] = '\\';
                output[output_pos++] = '\\';
                break;
            case '\n':
                if (output_pos + 2 >= output_size) goto buffer_full;
                output[output_pos++] = '\\';
                output[output_pos++] = 'n';
                break;
            case '\r':
                if (output_pos + 2 >= output_size) goto buffer_full;
                output[output_pos++] = '\\';
                output[output_pos++] = 'r';
                break;
            case '\t':
                if (output_pos + 2 >= output_size) goto buffer_full;
                output[output_pos++] = '\\';
                output[output_pos++] = 't';
                break;
            default:
                if (output_pos + 1 >= output_size) goto buffer_full;
                output[output_pos++] = c;
                break;
        }
    }

    output[output_pos] = '\0';
    return 0;

buffer_full:
    output[output_size - 1] = '\0';
    return -1;
}

/**
 * @brief HTTP 输出函数（使用 curl 命令发送）
 * @param message 告警消息
 * @param config 输出配置
 * @return 成功返回0，失败返回-1
 */
int linx_alert_output_http(linx_alert_message_t *message, linx_output_config_t *config)
{
    if (!message || !config || config->type != LINX_OUTPUT_TYPE_HTTP) {
        return -1;
    }

    const char *url = config->config.http_config.url;
    if (!url) {
        return -1;
    }

    // 格式化时间戳
    char timestamp_buf[64];
    if (format_iso8601_timestamp(message->timestamp, timestamp_buf, sizeof(timestamp_buf)) != 0) {
        strcpy(timestamp_buf, "unknown");
    }

    // 转义 JSON 字符串
    char escaped_message[2048];
    char escaped_rule_name[256];
    
    if (escape_json_string(message->formatted_message, escaped_message, sizeof(escaped_message)) != 0) {
        strcpy(escaped_message, "message too long");
    }

    if (message->rule_name) {
        if (escape_json_string(message->rule_name, escaped_rule_name, sizeof(escaped_rule_name)) != 0) {
            strcpy(escaped_rule_name, "unknown");
        }
    } else {
        strcpy(escaped_rule_name, "unknown");
    }

    // 构建 JSON 消息
    char json_data[4096];
    snprintf(json_data, sizeof(json_data),
        "{"
        "\"timestamp\":\"%s\","
        "\"priority\":\"%s\","
        "\"rule_name\":\"%s\","
        "\"message\":\"%s\","
        "\"source\":\"linx\""
        "}",
        timestamp_buf,
        get_priority_name(message->priority),
        escaped_rule_name,
        escaped_message
    );

    // 构建 curl 命令
    char curl_command[8192];
    int command_len = snprintf(curl_command, sizeof(curl_command),
        "curl -s -X POST -H \"Content-Type: application/json\"");

    // 添加自定义头部
    if (config->config.http_config.headers) {
        command_len += snprintf(curl_command + command_len, 
                               sizeof(curl_command) - command_len,
                               " -H \"%s\"", config->config.http_config.headers);
    }

    // 添加超时设置
    if (config->config.http_config.timeout_ms > 0) {
        int timeout_seconds = config->config.http_config.timeout_ms / 1000;
        if (timeout_seconds == 0) timeout_seconds = 1;
        command_len += snprintf(curl_command + command_len,
                               sizeof(curl_command) - command_len,
                               " --max-time %d", timeout_seconds);
    }

    // 添加数据和URL
    command_len += snprintf(curl_command + command_len,
                           sizeof(curl_command) - command_len,
                           " -d '%s' \"%s\" >/dev/null 2>&1",
                           json_data, url);

    // 检查命令长度
    if (command_len >= sizeof(curl_command)) {
        return -1;  // 命令太长
    }

    // 执行 curl 命令
    int result = system(curl_command);
    
    // 检查执行结果（简单检查）
    return (result == 0) ? 0 : -1;
}