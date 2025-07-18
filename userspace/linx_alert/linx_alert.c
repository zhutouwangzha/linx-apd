#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "linx_alert.h"
#include "linx_thread_pool.h"
#include "output_match_func.h"

/* 全局告警系统实例 */
linx_alert_system_t *g_alert_system = NULL;

/* 异步输出任务的参数结构 */
typedef struct {
    linx_alert_message_t *message;
} linx_alert_task_arg_t;

/* 前向声明内部函数 */
static void *linx_alert_worker_task(void *arg, int *should_stop);
static int linx_alert_send_to_outputs(linx_alert_message_t *message);
static char *linx_alert_format_timestamp(time_t timestamp);

/**
 * @brief 初始化告警系统
 * @param thread_pool_size 线程池大小，如果为0则使用默认值4
 * @return 成功返回0，失败返回-1
 */
int linx_alert_init(int thread_pool_size)
{
    if (g_alert_system != NULL) {
        return 0;  // 已经初始化
    }

    // 分配告警系统结构
    g_alert_system = malloc(sizeof(linx_alert_system_t));
    if (!g_alert_system) {
        return -1;
    }

    memset(g_alert_system, 0, sizeof(linx_alert_system_t));

    // 设置默认线程池大小
    if (thread_pool_size <= 0) {
        thread_pool_size = 4;
    }

    // 创建线程池
    g_alert_system->thread_pool = linx_thread_pool_create(thread_pool_size);
    if (!g_alert_system->thread_pool) {
        free(g_alert_system);
        g_alert_system = NULL;
        return -1;
    }

    // 初始化互斥锁
    if (pthread_mutex_init(&g_alert_system->config_mutex, NULL) != 0) {
        linx_thread_pool_destroy(g_alert_system->thread_pool, 1);
        free(g_alert_system);
        g_alert_system = NULL;
        return -1;
    }

    if (pthread_mutex_init(&g_alert_system->stats_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_alert_system->config_mutex);
        linx_thread_pool_destroy(g_alert_system->thread_pool, 1);
        free(g_alert_system);
        g_alert_system = NULL;
        return -1;
    }

    g_alert_system->initialized = true;
    g_alert_system->total_alerts_sent = 0;
    g_alert_system->total_alerts_failed = 0;

    return 0;
}

/**
 * @brief 清理告警系统
 */
void linx_alert_cleanup(void)
{
    if (!g_alert_system) {
        return;
    }

    g_alert_system->initialized = false;

    // 销毁线程池
    if (g_alert_system->thread_pool) {
        linx_thread_pool_destroy(g_alert_system->thread_pool, 1);
    }

    // 清理输出配置
    pthread_mutex_lock(&g_alert_system->config_mutex);
    if (g_alert_system->output_configs) {
        for (int i = 0; i < g_alert_system->output_count; i++) {
            linx_output_config_t *config = &g_alert_system->output_configs[i];
            if (config->type == LINX_OUTPUT_TYPE_FILE && config->config.file_config.file_path) {
                free(config->config.file_config.file_path);
            } else if (config->type == LINX_OUTPUT_TYPE_HTTP) {
                if (config->config.http_config.url) {
                    free(config->config.http_config.url);
                }
                if (config->config.http_config.headers) {
                    free(config->config.http_config.headers);
                }
            }
        }
        free(g_alert_system->output_configs);
    }
    pthread_mutex_unlock(&g_alert_system->config_mutex);

    // 销毁互斥锁
    pthread_mutex_destroy(&g_alert_system->config_mutex);
    pthread_mutex_destroy(&g_alert_system->stats_mutex);

    free(g_alert_system);
    g_alert_system = NULL;
}

/**
 * @brief 添加输出配置
 * @param config 输出配置
 * @return 成功返回0，失败返回-1
 */
int linx_alert_add_output_config(linx_output_config_t *config)
{
    if (!g_alert_system || !config) {
        return -1;
    }

    pthread_mutex_lock(&g_alert_system->config_mutex);

    // 检查是否已存在相同类型的配置
    for (int i = 0; i < g_alert_system->output_count; i++) {
        if (g_alert_system->output_configs[i].type == config->type) {
            pthread_mutex_unlock(&g_alert_system->config_mutex);
            return -1;  // 已存在
        }
    }

    // 扩展配置数组
    linx_output_config_t *new_configs = realloc(g_alert_system->output_configs, 
                                                (g_alert_system->output_count + 1) * sizeof(linx_output_config_t));
    if (!new_configs) {
        pthread_mutex_unlock(&g_alert_system->config_mutex);
        return -1;
    }

    g_alert_system->output_configs = new_configs;
    memcpy(&g_alert_system->output_configs[g_alert_system->output_count], config, sizeof(linx_output_config_t));
    g_alert_system->output_count++;

    pthread_mutex_unlock(&g_alert_system->config_mutex);
    return 0;
}

/**
 * @brief 异步发送告警
 * @param output_match 输出匹配对象
 * @param rule_name 规则名称
 * @param priority 优先级
 * @return 成功返回0，失败返回-1
 */
int linx_alert_send_async(linx_output_match_t *output_match, const char *rule_name, int priority)
{
    if (!g_alert_system || !g_alert_system->initialized || !output_match) {
        return -1;
    }

    // 格式化消息
    char buffer[4096];
    int format_result = linx_output_match_format(output_match, buffer, sizeof(buffer));
    if (format_result < 0) {
        return -1;
    }

    // 创建告警消息
    linx_alert_message_t *message = linx_alert_message_create(buffer, rule_name, priority);
    if (!message) {
        return -1;
    }

    // 复制输出配置
    pthread_mutex_lock(&g_alert_system->config_mutex);
    if (g_alert_system->output_count > 0) {
        message->output_configs = malloc(g_alert_system->output_count * sizeof(linx_output_config_t));
        if (message->output_configs) {
            memcpy(message->output_configs, g_alert_system->output_configs, 
                   g_alert_system->output_count * sizeof(linx_output_config_t));
            message->output_count = g_alert_system->output_count;
        }
    }
    pthread_mutex_unlock(&g_alert_system->config_mutex);

    // 创建任务参数
    linx_alert_task_arg_t *task_arg = malloc(sizeof(linx_alert_task_arg_t));
    if (!task_arg) {
        linx_alert_message_destroy(message);
        return -1;
    }
    task_arg->message = message;

    // 提交到线程池
    int result = linx_thread_pool_add_task(g_alert_system->thread_pool, linx_alert_worker_task, task_arg);
    if (result != 0) {
        free(task_arg);
        linx_alert_message_destroy(message);
        return -1;
    }

    return 0;
}

/**
 * @brief 同步发送告警
 * @param output_match 输出匹配对象
 * @param rule_name 规则名称
 * @param priority 优先级
 * @return 成功返回0，失败返回-1
 */
int linx_alert_send_sync(linx_output_match_t *output_match, const char *rule_name, int priority)
{
    if (!g_alert_system || !g_alert_system->initialized || !output_match) {
        return -1;
    }

    // 格式化消息
    char buffer[4096];
    int format_result = linx_output_match_format(output_match, buffer, sizeof(buffer));
    if (format_result < 0) {
        return -1;
    }

    // 创建告警消息
    linx_alert_message_t *message = linx_alert_message_create(buffer, rule_name, priority);
    if (!message) {
        return -1;
    }

    // 复制输出配置
    pthread_mutex_lock(&g_alert_system->config_mutex);
    if (g_alert_system->output_count > 0) {
        message->output_configs = malloc(g_alert_system->output_count * sizeof(linx_output_config_t));
        if (message->output_configs) {
            memcpy(message->output_configs, g_alert_system->output_configs, 
                   g_alert_system->output_count * sizeof(linx_output_config_t));
            message->output_count = g_alert_system->output_count;
        }
    }
    pthread_mutex_unlock(&g_alert_system->config_mutex);

    // 直接发送
    int result = linx_alert_send_to_outputs(message);
    
    linx_alert_message_destroy(message);
    return result;
}

/**
 * @brief 格式化并发送告警（推荐使用的主要接口）
 * @param output_match 输出匹配对象
 * @param rule_name 规则名称
 * @param priority 优先级
 * @return 成功返回0，失败返回-1
 */
int linx_alert_format_and_send(linx_output_match_t *output_match, const char *rule_name, int priority)
{
    // 默认使用异步发送，性能更好
    return linx_alert_send_async(output_match, rule_name, priority);
}

/**
 * @brief 工作线程任务函数
 * @param arg 任务参数
 * @param should_stop 停止标志
 * @return 始终返回NULL
 */
static void *linx_alert_worker_task(void *arg, int *should_stop)
{
    linx_alert_task_arg_t *task_arg = (linx_alert_task_arg_t *)arg;
    
    if (task_arg && task_arg->message) {
        linx_alert_send_to_outputs(task_arg->message);
        linx_alert_message_destroy(task_arg->message);
    }
    
    free(task_arg);
    return NULL;
}

/**
 * @brief 发送消息到所有配置的输出
 * @param message 告警消息
 * @return 成功返回0，失败返回-1
 */
static int linx_alert_send_to_outputs(linx_alert_message_t *message)
{
    if (!message || !message->output_configs) {
        return -1;
    }

    int success_count = 0;
    int total_count = message->output_count;

    for (int i = 0; i < message->output_count; i++) {
        linx_output_config_t *config = &message->output_configs[i];
        
        if (!config->enabled) {
            continue;
        }

        int result = -1;
        switch (config->type) {
            case LINX_OUTPUT_TYPE_STDOUT:
                result = linx_alert_output_stdout(message, config);
                break;
            case LINX_OUTPUT_TYPE_FILE:
                result = linx_alert_output_file(message, config);
                break;
            case LINX_OUTPUT_TYPE_HTTP:
                result = linx_alert_output_http(message, config);
                break;
            case LINX_OUTPUT_TYPE_SYSLOG:
                result = linx_alert_output_syslog(message, config);
                break;
            default:
                continue;
        }

        if (result == 0) {
            success_count++;
        }
    }

    // 更新统计信息
    pthread_mutex_lock(&g_alert_system->stats_mutex);
    if (success_count > 0) {
        g_alert_system->total_alerts_sent++;
    }
    if (success_count < total_count) {
        g_alert_system->total_alerts_failed++;
    }
    pthread_mutex_unlock(&g_alert_system->stats_mutex);

    return (success_count > 0) ? 0 : -1;
}

/**
 * @brief 创建告警消息对象
 * @param formatted_message 格式化后的消息
 * @param rule_name 规则名称
 * @param priority 优先级
 * @return 成功返回消息对象，失败返回NULL
 */
linx_alert_message_t *linx_alert_message_create(const char *formatted_message, const char *rule_name, int priority)
{
    if (!formatted_message) {
        return NULL;
    }

    linx_alert_message_t *message = malloc(sizeof(linx_alert_message_t));
    if (!message) {
        return NULL;
    }

    memset(message, 0, sizeof(linx_alert_message_t));

    // 复制消息内容
    message->message_len = strlen(formatted_message);
    message->formatted_message = malloc(message->message_len + 1);
    if (!message->formatted_message) {
        free(message);
        return NULL;
    }
    strcpy(message->formatted_message, formatted_message);

    // 复制规则名称
    if (rule_name) {
        message->rule_name = malloc(strlen(rule_name) + 1);
        if (message->rule_name) {
            strcpy(message->rule_name, rule_name);
        }
    }

    message->priority = priority;
    message->timestamp = time(NULL);

    return message;
}

/**
 * @brief 销毁告警消息对象
 * @param message 告警消息
 */
void linx_alert_message_destroy(linx_alert_message_t *message)
{
    if (!message) {
        return;
    }

    if (message->formatted_message) {
        free(message->formatted_message);
    }

    if (message->rule_name) {
        free(message->rule_name);
    }

    if (message->output_configs) {
        free(message->output_configs);
    }

    free(message);
}

/**
 * @brief 获取统计信息
 * @param total_sent 总发送数
 * @param total_failed 总失败数
 */
void linx_alert_get_stats(long *total_sent, long *total_failed)
{
    if (!g_alert_system) {
        if (total_sent) *total_sent = 0;
        if (total_failed) *total_failed = 0;
        return;
    }

    pthread_mutex_lock(&g_alert_system->stats_mutex);
    if (total_sent) *total_sent = g_alert_system->total_alerts_sent;
    if (total_failed) *total_failed = g_alert_system->total_alerts_failed;
    pthread_mutex_unlock(&g_alert_system->stats_mutex);
}

/**
 * @brief 重置统计信息
 */
void linx_alert_reset_stats(void)
{
    if (!g_alert_system) {
        return;
    }

    pthread_mutex_lock(&g_alert_system->stats_mutex);
    g_alert_system->total_alerts_sent = 0;
    g_alert_system->total_alerts_failed = 0;
    pthread_mutex_unlock(&g_alert_system->stats_mutex);
}
