#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "linx_alert.h"

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
 * @brief 获取文件大小
 * @param filepath 文件路径
 * @return 文件大小，失败返回-1
 */
static long get_file_size(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/**
 * @brief 执行日志轮转
 * @param filepath 原始文件路径
 * @param max_files 最大文件数
 * @return 成功返回0，失败返回-1
 */
static int rotate_log_file(const char *filepath, int max_files)
{
    if (!filepath || max_files <= 0) {
        return -1;
    }

    char old_name[512];
    char new_name[512];

    // 删除最老的文件（如果存在）
    snprintf(old_name, sizeof(old_name), "%s.%d", filepath, max_files);
    unlink(old_name);  // 忽略错误

    // 向后移动所有编号的文件
    for (int i = max_files - 1; i >= 1; i--) {
        snprintf(old_name, sizeof(old_name), "%s.%d", filepath, i);
        snprintf(new_name, sizeof(new_name), "%s.%d", filepath, i + 1);
        rename(old_name, new_name);  // 忽略错误
    }

    // 移动当前文件为 .1
    snprintf(new_name, sizeof(new_name), "%s.1", filepath);
    if (rename(filepath, new_name) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief 检查是否需要日志轮转
 * @param filepath 文件路径
 * @param max_size 最大文件大小
 * @param max_files 最大文件数
 * @return 需要轮转返回1，不需要返回0，错误返回-1
 */
static int should_rotate_log(const char *filepath, size_t max_size, int max_files)
{
    if (!filepath || max_size == 0) {
        return 0;
    }

    long current_size = get_file_size(filepath);
    if (current_size < 0) {
        return 0;  // 文件不存在，不需要轮转
    }

    return (current_size >= max_size) ? 1 : 0;
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
 * @brief 文件输出函数
 * @param message 告警消息
 * @param config 输出配置
 * @return 成功返回0，失败返回-1
 */
int linx_alert_output_file(linx_alert_message_t *message, linx_output_config_t *config)
{
    if (!message || !config || config->type != LINX_OUTPUT_TYPE_FILE) {
        return -1;
    }

    const char *filepath = config->config.file_config.file_path;
    if (!filepath) {
        return -1;
    }

    // 检查是否需要日志轮转
    if (config->config.file_config.max_file_size > 0 && 
        config->config.file_config.max_files > 0) {
        
        if (should_rotate_log(filepath, config->config.file_config.max_file_size, 
                             config->config.file_config.max_files) == 1) {
            
            if (rotate_log_file(filepath, config->config.file_config.max_files) != 0) {
                // 轮转失败，但继续尝试写入
            }
        }
    }

    // 确定打开模式
    const char *mode = config->config.file_config.append ? "a" : "w";
    
    FILE *file = fopen(filepath, mode);
    if (!file) {
        return -1;
    }

    // 格式化时间戳
    char timestamp_buf[64];
    if (format_timestamp(message->timestamp, timestamp_buf, sizeof(timestamp_buf)) != 0) {
        strcpy(timestamp_buf, "UNKNOWN");
    }

    // 写入日志条目
    fprintf(file, "[%s] [%s]", timestamp_buf, get_priority_name(message->priority));
    
    if (message->rule_name) {
        fprintf(file, " [%s]", message->rule_name);
    }
    
    fprintf(file, " %s\n", message->formatted_message);
    
    // 确保数据写入磁盘
    fflush(file);
    fsync(fileno(file));
    
    fclose(file);

    return 0;
}